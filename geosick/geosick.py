from dataclasses import dataclass
from typing import List, Iterator, Optional, Tuple

## API

# Sample of a user location. The samples for a user must be ordered by timestamp
# (increasing).
#
# We assume that the samples are taken with a steady rate, so we can reasonably
# interpolate between samples. To indicate that there is a gap in the data (i.e., the GPS
# signal was lost or the collection was stopped), mark the last sample before the gap with
# `is_end = True`: we will assume that there is no knowledge of the user position between
# the sample with `is_gap = True` and the following sample.
#
# We also assume that we don't know anything about the position before the first sample
# and after the last sample.
#
# The horizontal and vertical accuracy are assumed to be maximum "reasonable" distance of
# the true position from the indicated position: it is NOT a standard deviation.
@dataclass
class UserSample:
    timestamp_ms: int # Timestamp in ms (unix epoch)
    latitude_e7: int # Latitude in deg * 1e7
    longitude_e7: int # Longitude in deg * 1e7
    accuracy_m: float # Horizontal position accuracy in meters
    velocity_mps: Optional[float] # Horizontal speed in meters per second
    heading_deg: Optional[float] # Direction of horizontal velocity in degrees (north 0, east 90)
    is_end:  bool # If true, this sample starts a gap: we don't know where the user is until
        # the next sample

# Request for an infection analysis.
@dataclass
class Request:
    sick_samples: List[UserSample] # Samples of the infected person.
    query_samples: List[UserSample] # Samples of the queried person

# Result of the infection analysis.
@dataclass
class Response:
    score: float # Infection score (higher score -> higher likelihood of infection)
    distance: float # Conservative estimate of the minimum distance between the two
        # persons

def analyze(request: Request) -> Response:
    sick_samples, query_samples = request.sick_samples, request.query_samples
    if len(sick_samples) < 2:
        raise ArgumentError("There are not enough sick_samples")
    if  len(query_samples) < 2:
        raise ArgumentError("There are not enough query_samples")

    start_timestamp = max(sick_samples[0].timestamp_ms, query_samples[0].timestamp_ms)
    end_timestamp = min(sick_samples[-1].timestamp_ms, query_samples[-1].timestamp_ms)
    if start_timestamp >= end_timestamp:
        raise ArgumentError("sick_samples and query_samples do not intersect in time")

    ctx = Ctx()
    ctx.period_s = 30
    ctx.ne_origin = (sick_samples[0].latitude_e7, sick_samples[0].longitude_e7)
    timestamps = list(range(start_timestamp, end_timestamp, period*1000))
    sick_points = interpolate(ctx, sick_samples, timestamps)
    query_points = interpolate(ctx, query_samples, timestamps)
    response = meet(sick_points, query_points, period_s)
    return response


## Interpolation

class Ctx:
    request: Request
    period_s: float
    ne_origin: Tuple[int, int]

@dataclass
class Point:
    pos: np.array, # North-east position in meters
    altitude: float # Altitude in meters
    radius: float # Horizontal accuracy in meters
    velocity: Optional[np.array] # North-east velocity in meters per second

PointStream = Iterator[Optional[Point]]

def interpolate(ctx: Ctx, samples: List[UserSample], timestamps: List[int]) -> PointStream:
    curr_sample, curr_i = samples[0], 0
    next_sample = samples[1]
    for timestamp_ms in timestamps:
        while timestamp_ms >= next_sample.timestamp_ms:
            curr_sample, next_sample = next_sample, samples[curr_i+1]
            curr_i += 1
        assert curr_sample.timestamp_ms <= timestamp_ms < next_sample.timestamp_ms

# Converts UserSample to a Point
def sample_to_point(ctx, sample):
    northing, easting, altitude = geo_to_nea(ctx.ne_origin,
        sample.latitude_e7, sample.longitude_e7, sample.altitude_m)
    pos = np.array([northing, easting])
    radius = sample.accuracy_m

    if sample.velocity_mps is not None and sample.heading_deg is not None:
        heading_rad = sample.heading_deg * np.pi / 180
        heading_ne = np.array([cos(heading_rad), sin(heading_rad)])
        velocity = heading_ne * sample.velocity_mps
    else:
        velocity = None

    return Point(pos=pos, altitude=altitude, radius=radius, velocity=velocity)

# Linearly interpolates two Point-s as `p0*(1-alpha) + p1`
def lerp_points(ctx, p0, p1, alpha):
    pos = 

# Converts latitude, longitude and altitude to northing, easting, altitude. The
# latitude-longitude pair `ne_origin` is used as the origin of the northing-easing
# coordinate system (it should be reasonably close to the given coordinates).
def geo_to_nea(ne_origin, latitude_e7, longitude_e7, altitude_m):
    lat_deg_e7_to_m = 0.0111132 # at latitude 45 deg, negligible variation with latitude
    lat_rad = latitude_e7 / 174533
    lng_deg_e7_to_m = 0.0111319 * np.cos(lat_rad)

    northing_m = latitude_e7 * lat_deg_e7_to_m
    easting_m = longitude_e7 * lng_deg_e7_to_m
    return (northing_m, easting_m, altitude_m)


def meet(sick_points: PointStream, query_points: PointStream,
        period_s: float) -> Response:
    pass




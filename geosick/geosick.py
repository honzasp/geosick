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
    velocity_mps: float # Horizontal speed in meters per second
    heading_deg: float # Direction of horizontal velocity in degrees (north 0, east 90)
    altitude_m: float # WGS84 altitude in meters
    vertical_accuracy_m: float # Vertical accuracy in meters
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
    if not sick_samples:
        raise ArgumentError("There are no sick_samples")
    if not query_samples:
        raise ArgumentError("There are no query_samples")

    start_timestamp = max(sick_samples[0].timestamp_ms, query_samples[0].timestamp_ms)
    end_timestamp = min(sick_samples[-1].timestamp_ms, query_samples[-1].timestamp_ms)
    if start_timestamp >= end_timestamp:
        raise ArgumentError("sick_samples and query_samples do not intersect in time")

    period_s = 30
    timestamps = range(start_timestamp, end_timestamp, period*1000)
    sick_points = interpolate(sick_samples, timestamps)
    query_points = interpolate(query_samples, timestamps)
    response = meet(sick_points, query_points, period_s)
    return response

##

@dataclass
class Point:
    northing_m: float # North position in meters
    easting_m: float # East position in meters
    altitude_m: float # Altitude in meters
    radius_m: float # Horizontal accuracy in meters
    velocity_mps: Optional[Tuple[float, float]] # Horizontal velocity in meters per second

def interpolate(samples: List[UserSample], timestamps: List[int]) -> Iterator[Point]:
    pass

def meet(sick_points: Iterator[Point], query_points: Iterator[Point],
        period_s: float) -> Response:
    pass




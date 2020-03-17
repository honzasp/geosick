from typing import List
from .geosick import Ctx, UserSample, Point, PointStream
import numpy as np

# Minimum Point.radius in meters
MIN_RADIUS = 5.0
# Maximum Point.velocity in maters per second
MAX_SPEED = 90 / 3.6
# Maximum allowable distance for interpolation in meters
MAX_DELTA_DISTANCE = 100.0
# Maximum allowable time duration for interpolation between two points that are more than
# MAX_CLOSE_DISTANCE apart, in seconds
MAX_DELTA_TIME = 5*60
# Maximum distance between two points that is considered "close", in meters
MAX_CLOSE_DISTANCE = 5.0

# Interpolates a list of samples into a stream of points at ctx.timestamps_ms
def interpolate(ctx: Ctx, samples: List[UserSample]) -> PointStream:
    curr_sample, curr_i = samples[0], 0
    curr_point = sample_to_point(ctx, curr_sample)
    next_sample = samples[1]
    next_point = sample_to_point(ctx, next_sample)
    for timestamp_ms in ctx.timestamps_ms:
        while timestamp_ms >= next_sample.timestamp_ms:
            curr_sample, next_sample = next_sample, samples[curr_i+1]
            curr_point, next_point = next_point, sample_to_point(ctx, next_sample)
            curr_i += 1
        assert curr_sample.timestamp_ms <= timestamp_ms < next_sample.timestamp_ms

        if curr_sample.is_end:
            point = None
        else:
            point = lerp_points(ctx, curr_point, next_point, timestamp_ms)
        yield point

# Converts UserSample to a Point
def sample_to_point(ctx, sample):
    northing, easting = geo_to_ne(ctx.ne_origin, sample.latitude_e7, sample.longitude_e7)
    pos = np.array([northing, easting])
    radius = max(sample.accuracy_m, MIN_RADIUS)

    if sample.velocity_mps is not None and sample.heading_deg is not None:
        heading_rad = sample.heading_deg * np.pi / 180
        heading_ne = np.array([np.cos(heading_rad), np.sin(heading_rad)])
        velocity = heading_ne * sample.velocity_mps
    else:
        velocity = None

    return Point(timestamp=sample.timestamp_ms,
        pos=pos, radius=radius, velocity=velocity)

# Linearly interpolates between two points at given timestamp
def lerp_points(ctx, p0, p1, timestamp_ms):
    distance = np.linalg.norm(p1.pos - p0.pos)
    delta_t_s = (p1.timestamp - p0.timestamp) / 1000

    if distance > MAX_DELTA_DISTANCE:
        return None
    if delta_t_s > MAX_DELTA_TIME and distance > MAX_CLOSE_DISTANCE:
        return None

    alpha = (timestamp_ms - p0.timestamp) / (p1.timestamp - p0.timestamp)
    assert 0 <= alpha <= 1
    pos = p0.pos*(1-alpha) + p1.pos*alpha
    radius = p0.radius*(1-alpha) + p1.radius*alpha

    if p0.velocity is not None and p1.velocity is not None:
        velocity = p0.velocity*(1-alpha) + p1.velocity*alpha
    else:
        velocity = (p1.pos - p0.pos) / max(0.1, delta_t_s)

    speed = np.linalg.norm(velocity)
    if speed > MAX_SPEED:
        velocity *= MAX_SPEED / speed

    return Point(timestamp=timestamp_ms,
        pos=pos, radius=radius, velocity=velocity)

# Converts latitude, longitude to northing, easting. The latitude-longitude pair
# `ne_origin` is used as the origin of the northing-easing coordinate system (it should be
# reasonably close to the given coordinates).
def geo_to_ne(ne_origin, latitude_e7, longitude_e7):
    lat_deg_e7_to_m = 0.0111132 # at latitude 45 deg, negligible variation with latitude
    lat_rad = latitude_e7 / 174533
    lng_deg_e7_to_m = 0.0111319 * np.cos(lat_rad)

    northing_m = (latitude_e7 - ne_origin[0]) * lat_deg_e7_to_m
    easting_m = (longitude_e7 - ne_origin[1]) * lng_deg_e7_to_m
    return (northing_m, easting_m)



from typing import List
from .geosick import Ctx, UserSample, PointStream
import numpy as np

# Interpolates a list of samples into a stream of points at the given timestamps
def interpolate(ctx: Ctx, samples: List[UserSample], timestamps: List[int]) -> PointStream:
    curr_sample, curr_i = samples[0], 0
    curr_point = sample_to_point(ctx, curr_sample)
    next_sample = samples[1]
    next_point = sample_to_point(ctx, next_sample)
    for timestamp_ms in timestamps:
        while timestamp_ms >= next_sample.timestamp_ms:
            curr_sample, next_sample = next_sample, samples[curr_i+1]
            curr_point, next_point = next_point, sample_to_point(ctx, next_sample)
            curr_i += 1
        assert curr_sample.timestamp_ms <= timestamp_ms < next_sample.timestamp_ms

        if not curr_sample.is_end:
            point = lerp_points(ctx, curr_point, next_point, timestamp_ms)
        else:
            point = None
        yield point

# Converts UserSample to a Point
def sample_to_point(ctx, sample):
    northing, easting = geo_to_ne(ctx.ne_origin, sample.latitude_e7, sample.longitude_e7)
    pos = np.array([northing, easting])
    radius = sample.accuracy_m

    if sample.velocity_mps is not None and sample.heading_deg is not None:
        heading_rad = sample.heading_deg * np.pi / 180
        heading_ne = np.array([cos(heading_rad), sin(heading_rad)])
        velocity = heading_ne * sample.velocity_mps
    else:
        velocity = None

    return Point(pos=pos, radius=radius, velocity=velocity)

# Linearly interpolates between two points at given timestamp
def lerp_points(ctx, p0, p1, timestamp_ms):
    alpha = (timestamp_ms - p0.timestamp_ms) / (p1.timestamp - p0.timestamp)
    assert 0 <= alpha <= 1
    pos = p0.pos*(1-alpha) + p1.pos*alpha
    radius = p0.radius*(1-alpha) + p1.radius*alpha

    if p0.velocity is not None and p1.velocity is not None:
        velocity = p0.velocity*(1-alpha) + p1.velocity*alpha
    else:
        delta_t_s = (p1.timestamp_ms - p0.timestamp_ms) / 1000
        velocity = (p1.pos - p0.pos) / delta_t_s

    return Point(pos=pos, radius=radius, velocity=velocity)

# Converts latitude, longitude to northing, easting. The latitude-longitude pair
# `ne_origin` is used as the origin of the northing-easing coordinate system (it should be
# reasonably close to the given coordinates).
def geo_to_ne(ne_origin, latitude_e7, longitude_e7):
    lat_deg_e7_to_m = 0.0111132 # at latitude 45 deg, negligible variation with latitude
    lat_rad = latitude_e7 / 174533
    lng_deg_e7_to_m = 0.0111319 * np.cos(lat_rad)

    northing_m = latitude_e7 * lat_deg_e7_to_m
    easting_m = longitude_e7 * lng_deg_e7_to_m
    return (northing_m, easting_m)



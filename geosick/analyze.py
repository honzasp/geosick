from dataclasses import dataclass
from typing import List, Optional, Tuple
from .geosick import Request, Response, Ctx
from .interpolate import interpolate
from .meet import meet
import numpy as np

def analyze(request: Request) -> Response:
    sick_samples, query_samples = request.sick_samples, request.query_samples
    if len(sick_samples) < 2:
        raise ValueError("There are not enough sick_samples")
    if len(query_samples) < 2:
        raise ValueError("There are not enough query_samples")

    start_timestamp = max(sick_samples[0].timestamp_ms, query_samples[0].timestamp_ms)
    end_timestamp = min(sick_samples[-1].timestamp_ms, query_samples[-1].timestamp_ms)
    if start_timestamp >= end_timestamp:
        raise ArgumentError("sick_samples and query_samples do not intersect in time")

    ctx = Ctx()
    ctx.request = request
    ctx.period_ms = 10000
    ctx.ne_origin = (sick_samples[0].latitude_e7, sick_samples[0].longitude_e7)
    ctx.timestamps_ms = list(range(start_timestamp, end_timestamp, ctx.period_ms))
    sick_points = interpolate(ctx, sick_samples)
    query_points = interpolate(ctx, query_samples)
    response = meet(ctx, sick_points, query_points)
    return response

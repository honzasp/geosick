import numpy as np

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


## Internal structures

# Request-specific context
class Ctx:
    request: Request
    period_s: float
    ne_origin: Tuple[int, int]

@dataclass
class Point:
    pos: np.array # North-east position in meters
    radius: float # Horizontal accuracy in meters
    velocity: Optional[np.array] # North-east velocity in meters per second; estimated if not
        # available

PointStream = Iterator[Optional[Point]]

## Interpolation

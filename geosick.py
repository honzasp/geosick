import collections

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
# The horizontal and vertical accuracy are assumed to be maximum "reasonable" distance of
# the true position from the indicated position: it is NOT a standard deviation.
UserSample = collections.namedtuple("UserSample", [
    "timestamp", # Timestamp in ms (unix epoch)
    "latitude_e7", # Latitude in deg * 1e7
    "longitude_e7", # Longitude in deg * 1e7
    "accuracy", # Horizontal position accuracy in meters
    "velocity", # Horizontal speed in meters per second
    "heading", # Direction of horizontal velocity in degrees (north 0, east 90)
    "altitude", # WGS84 altitude in meters
    "vertical_accuracy", # Vertical accuracy in meters
    "is_end", # If true, this sample starts a gap: we don't know where the user is until
        # the next sample
])

# Request for an infection analysis.
Request = collections.namedtuple("Request", [
    "sick_samples", # List of UserSample-s of the infected person.
    "query_samples", # List of UserSample-s of the queried person
])

# Result of the infection analysis.
Response = collections.namedtuple("Response", [
    "min_distance", # Conservative estimate of the distance
    "score", # Infection score (higher score -> higher likelihood of infection)
])

def analyze(request: Request) -> Response:
    raise NotImplementedError()

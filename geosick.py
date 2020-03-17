import collections

## API

# Sample of a user location.
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

# Request for an infection analysis
Request = collections.namedtuple("Request", [
    "sick_samples", # List of UserSample-s of the infected person.
    "query_samples", # List of UserSample-s of the queried person
])

# Result of the infection analysis
Response = collections.namedtuple("Response", [
    "min_distance", # Distance of the nearest contact of the two persons
    "score", # Infection score (higher score -> higher likelihood of infection)
])

def analyze(request: Request) -> Response:
    raise NotImplementedError()

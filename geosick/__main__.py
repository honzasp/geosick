"""
Usage: skystore [-p <listen-port>]

Options:
   -p, --port <listen-port>
      Specifies the port where the server will listen for HTTP requests. Default is 4100

   Geosick service provides a simple HTTP interface for evaluation of probability
   of one person infecting another based on their geolocation time sequence.

   Geosick lives at https://https://github.com/honzasp/geosick,
   the responsible persons are Jan Plhák (jan.plhak@kiwi.com) and
   Jan Špaček (patek.mail@gmail.com).
"""
import bottle
import docopt
import ujson

from .geosick import UserSample, Request, Response
from .analyze import analyze

app = bottle.Bottle()

def sample_from_json(geo_json) -> UserSample:
    return UserSample(
        timestamp_ms=int(geo_json["timestamp_ms"]),
        latitude_e7=int(geo_json["latitude_e7"]),
        longitude_e7=int(geo_json["longitude_e7"]),
        accuracy_m=float(geo_json["accuracy_m"]),
        velocity_mps=geo_json.get("velocity_mps"),
        heading_deg=geo_json.get("heading_deg"),
        is_end=geo_json.get("is_end", False),
    )

def request_from_json(req_json) -> Request:
    return Request(
        sick_samples=[
            sample_from_json(geopoint)
            for geopoint in req_json["sick_geopoints"]
        ],
        query_samples=[
            sample_from_json(geopoint)
            for geopoint in req_json["query_geopoints"]
        ]
    )

def response_to_json(resp: Response):
    return {
        "score": resp.score,
        "minimal_distance_m": resp.min_distance_m,
        "meeting_range_ms": resp.meet_ranges_ms,
    }

@app.post("/v1/evaluate_risk")
def evaluate_risk():
    try:
        request = request_from_json(ujson.load(bottle.request.body))
    except ValueError as e:
        bottle.response.status = 400
        return f"ERROR: the request is invalid: {e}"
    except KeyError as e:
        bottle.response.status = 400
        return f"ERROR: key error: {e}"
    response = analyze(request)
    return response_to_json(response)

if __name__ == "__main__":
    args = docopt.docopt(__doc__)
    bottle.run(app, host="0.0.0.0", port=int(args["--port"] or 4100))

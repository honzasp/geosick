"""
Usage: geosick [-p <listen-port>]

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
import os.path
import ujson

from .geosick import UserSample, Request, Response, Point, Step
from .analyze import analyze

app = bottle.Bottle()


## Deserialization from JSON

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
        ],
        full=req_json.get("full", False),
    )


## Serialization to JSON

def request_to_json(req):
    return {
        "sick_geopoints": [sample_to_json(s) for s in req.sick_samples],
        "query_geopoints": [sample_to_json(s) for s in req.query_samples],
        "full": req.full,
    }

def response_to_json(resp: Response):
    return {
        "score": resp.score,
        "minimal_distance_m": resp.min_distance_m,
        "meeting_ranges_ms": resp.meet_ranges_ms,
        "steps": [step_to_json(s) for s in resp.steps],
    }

def step_to_json(step: Step):
    return {
        "timestamp": step.timestamp,
        "sick_point": point_to_json(step.sick_point),
        "query_point": point_to_json(step.query_point),
        "risk": step.risk,
        "distance": step.distance,
    }

def point_to_json(point: Point):
    return {
        "timestamp": point.timestamp,
        "pos": list(point.pos),
        "radius": point.radius,
        "velocity": list(point.velocity) if point.velocity is not None else None,
        "latitude_e7": point.latitude_e7,
        "longitude_e7": point.longitude_e7,
    }

def sample_to_json(sample: UserSample):
    return {
        "timestamp_ms": sample.timestamp_ms,
        "latitude_e7": sample.latitude_e7,
        "longitude_e7": sample.longitude_e7,
        "accuracy_m": sample.accuracy_m,
        "velocity_mps": sample.velocity_mps,
        "heading_deg": sample.heading_deg,
        "is_end": sample.is_end,
    }


## HTTP application

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

@app.get("/")
def get_map():
    return bottle.static_file("map.html", os.path.dirname(__file__))

if __name__ == "__main__":
    args = docopt.docopt(__doc__)
    bottle.run(app, host="0.0.0.0", port=int(args["--port"] or 4100))

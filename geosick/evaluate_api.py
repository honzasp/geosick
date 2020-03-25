"""
Usage: geosick.evaluate_api [-p <listen-port>]

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

from .analyze import analyze
from .geosick import UserSample, Request, Response, Point, Step
from .json import request_from_json, response_to_json

app = bottle.Bottle()


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

"""
Usage: geosick.zostanzdravy <config-file>
"""
import docopt
import json
import os.path
import subprocess

from .analyze import analyze
from .json import request_from_json, response_to_json

EXECUTABLE = os.path.join(os.path.dirname(__file__), "../build/zostanzdravy")

if __name__ == "__main__":
    args = docopt.docopt(__doc__)
    config_path = args["<config-file>"]
    with open(config_path, "rt") as f:
        cfg = json.load(f)

    requests_path = os.path.join(cfg["temp_dir"], "requests.json")
    proc = subprocess.run([EXECUTABLE, config_path, requests_path])
    proc.check_returncode()

    with open(requests_path, "rt") as requests_f:
        for line in requests_f:
            request = request_from_json(json.loads(line))
            response = analyze(request)
            print(request)
            print(response)
            print()


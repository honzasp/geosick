import json
import numpy as np
import os.path
import xml.etree.ElementTree as ET

from .geosick import UserSample, Request
from .interpolate import geo_to_ne
from .__main__ import request_to_json

GPX_DIR = os.path.join(os.path.dirname(__file__), "../test/gpx")

def read_gpx(gpx_file):
    tree = ET.parse(gpx_file)
    for trkpt in tree.getroot().findall(".//{*}trkpt"):
        lat = float(trkpt.get("lat"))
        lng = float(trkpt.get("lon"))
        yield (lat*10000000, lng*10000000)

def gpx_to_samples(gpx_points, timestamp_ms, accuracy_m, speed_mps):
    ne_origin = gpx_points[0]
    ne_points = [geo_to_ne(ne_origin, lat, lng) for lat, lng in gpx_points]
    total_distance_m = sum(np.linalg.norm(p1 - p0)
        for p0, p1 in zip(ne_points[:-1], ne_points[1:]))
    total_duration_s = total_distance_m / speed_mps
    segment_duration_s = total_duration_s / (len(gpx_points) - 1)

    for i, (lat, lng) in enumerate(gpx_points):
        yield UserSample(
            timestamp_ms=int(timestamp_ms + i*segment_duration_s*1000),
            latitude_e7=lat,
            longitude_e7=lng,
            accuracy_m=accuracy_m,
            velocity_mps=None,
            heading_deg=None,
            is_end=False,
        )

if __name__ == "__main__":
    timestamp_ms = 1584520793000;
    gpx_points_1 = list(read_gpx(os.path.join(GPX_DIR, "vaclavak_1.gpx")))
    samples_1 = list(gpx_to_samples(gpx_points_1, timestamp_ms, 5.0, 6/3.6))
    gpx_points_2 = list(read_gpx(os.path.join(GPX_DIR, "vaclavak_2.gpx")))
    samples_2 = list(gpx_to_samples(gpx_points_2, timestamp_ms, 8.0, 5/3.6))
    req = Request(sick_samples=samples_1, query_samples=samples_2)
    print(json.dumps(request_to_json(req)))

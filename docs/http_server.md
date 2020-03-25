
## Geosick HTTP server
The Geosick HTTP server provides a simple [API](#api) with a sole purpose of analyzing
two geopoint sequencies (one sequence from a healthy person and one from a
person infected by the virus) and determine the probability that the healthy
specimen was infected. The service as such is written in Python and isn't intended for
big-scale sequence processing. For that, please see our
[batch processing docs.](docs/batch_processing.md)

## How to run the server

    python3.8 -m geosick.evaluate_api --help

## Algorithm description

![Description of the algoritm](docs/algorithm_description.png?raw=true "Algorithm description")

## API

The API is very simple and implements only one endpoint:

    POST /v1/evaluate_risk
    ->  <request>
    <-  <response>

where

    <request> = {
        "sick_geopoints": [<geopoint>, ...],
        "query_geopoints": [<geopoint>, ...],
    }

    <geopoint> = {
        "timestamp_ms": <integer>,
        "latitude_e7": <integer>,
        "longitude_e7": <integer>,
        "accuracy_m": <float>,
        "velocity_mps": <float>?,
        "heading_deg": <float>?,
        "is_end": <bool>?,
    }

    <response> = {
        "score": <float>,
        "minimal_distance_m": <integer>,
        "meeting_ranges_ms": [<meeting-range>, ...]
    }

    <meeting-range> = [<integer>, <integer>]

- `score` is the estimated probability of disease transmission.
- `minimal_distance_m` is the smallest distance the two people got to each other.
- `meeting_ranges_ms` represent time frames of possible contact between the two
  people (pretty much the time frames at which a transmission could occur with
  non-zero probability).

## Examples

Request:

    {
        "sick_geopoints": [
            {
                "timestamp_ms": "1521743776992",
                "latitude_e7" : 371227520,
                "longitude_e7" : -76565789,
                "accuracy_m" : 5,
                "velocity_mps" : 9,
                "heading_deg" : 274
            },
            {
                "timestamp_ms" : "1521743776993",
                "latitude_e7" : 371227520,
                "longitude_e7" : -76565789,
                "accuracy_m" : 5,
                "velocity_mps" : 9,
                "heading_deg" : 274
            }
        ],
        "query_geopoints": [
            {
                "timestamp_ms" : "1521743776992",
                "latitude_e7" : 371227520,
                "longitude_e7" : -76565789,
                "accuracy_m" : 5,
                "velocity_mps" : 9,
                "heading_deg" : 274
            },
            {
                "timestamp_ms" : "1521743776993",
                "latitude_e7" : 371227520,
                "longitude_e7" : -76565789,
                "accuracy_m" : 5,
                "velocity_mps" : 9,
                "heading_deg" : 274
            }
        ]
    }

Response:

    {
        "score": 0.89,
        "minimal_distance_m": 10,
        "meeting_ranges_ms": [
            [14800, 14900],
            [30000, 35000],
        ]
    }

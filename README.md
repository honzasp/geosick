## Geosick Service

Geosick is a service written for the Covid19cz initiative by Jan Plhak and Jan Spacek.
The purpose of the service is to analyze two Geopoint sequencies (one sequence from a healthy person
and one from a person infected by the Covid19 virus) and determine a probability that the healthy
specimen was infected.

## API

The API is very simplistic and implements only one endpoint:

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
        "accuracy_m": <integer>,
        "velocity_mps": <integer>?,
        "heading_deg": <integer>?,
        "is_end": <bool>?,
    }

    <response> = {
        "score": <float>,
        "minimal_distance_m": <integer>,
        "meeting_ranges_ms": [<meeting-range>, ...]
    }

    <meeting-range> = [<integer>, <integer>]

`score` is a probability of disease transmission.
`minimal_distance_m` is the smallest distance the two people got to each other.
`meeting_ranges_ms` represent time frames of possible contact between the two people
(pretty much the time frames at which a transmission could occur with non-zero probability).

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

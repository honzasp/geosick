import aiohttp
import aiohttp.web
import asyncio
import logging
import ujson

from geosick.geosick import UserSample, Request, Response
from geosick.analyze import analyze

logger = logging.getLogger("geosick.server")

async def run_server(executor, args):
    loop = asyncio.get_event_loop()
    port = int(args.get("--port") or 4100)

    async def post_eval_risk(req):
        try:
            req_json = ujson.loads(await req.text())
        except ValueError:
            return aiohttp.web.Response(
                text="ERROR: The JSON you have provided is invalid",
                status=400,
                headers={"Content-Type":"application/text"}
            )

        def geo_from_json(geo_json) -> UserSample:
            return UserSample(
                timestamp_ms=int(geo_json["timestamp_ms"]),
                latitude_e7=int(geo_json["latitude_e7"]),
                longitude_e7=int(geo_json["longitude_e7"]),
                accuracy_m=int(geo_json["accuracy_m"]),
                velocity_mps=geo_json.get("velocity_mps"),
                heading_deg=geo_json.get("heading_deg"),
                is_end=geo_json.get("is_end", False),
            )

        try:
            query = Request(
                sick_samples=[
                    geo_from_json(geopoint)
                    for geopoint in req_json["sick_geopoints"]
                ],
                query_samples=[
                    geo_from_json(geopoint)
                    for geopoint in req_json["query_geopoints"]
                ]
            )
        except KeyError as e:
            return aiohttp.web.Response(
                text=f"ERROR: Got JSON KeyError: missing key {e}",
                status=400,
                headers={"Content-Type":"application/text"}
            )

        resp = await loop.run_in_executor(executor, analyze, query)
        resp_json = {
            "score": resp.score,
            "minimal_distance": resp.distance
        }

        return aiohttp.web.Response(
            body=ujson.dumps(resp_json),
            status=200,
            headers={"Content-Type":"application/json"}
        )

    app = aiohttp.web.Application()
    app.router.add_route("POST", "/v1/evaluate_risk", post_eval_risk)

    runner = aiohttp.web.AppRunner(app)
    await runner.setup()
    site = aiohttp.web.TCPSite(runner, "0.0.0.0", port)
    app_task = loop.create_task(site.start())

    logger.info(f'Geosick now running on port {port}...')
    try:
        await asyncio.gather(app_task, asyncio.Future())
    finally:
        await runner.cleanup()

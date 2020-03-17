u"""
Usage: skystore [-p <listen-port>]

Options:
   -p, --port <listen-port>
      Specifies the port where the server will listen for HTTP requests. Default is 4100

   Geosick service provides a simple HTTP interface for evaluation of probability
   of one person infecting another based on their geolocation time sequence.

   Geosick lives at https://https://github.com/honzasp/geosick,
   the responsible persons are Jan Plhák (jan.plhak@kiwi.com) and
   Jan Špaček (patek.mail@gmai.com).
"""

import asyncio
import aiohttp
import aiohttp.web
import concurrent
import docopt
import logging
import sys
import signal
import ujson
import os

from asyncio.futures import CancelledError

from geosick.geosick import UserSample, Request, Response
from geosick.analyze import analyze

logger = logging.getLogger("geosick.server")

async def _run_server(executor, args):
    loop = asyncio.get_event_loop()
    port = int(args.get("--port") or 4100)

    async def post_eval_risk(req):
        req_json = ujson.loads(await req.text())

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

def _main(args):
    try:
        loop = asyncio.get_event_loop()

        thread_pool_size = int(os.cpu_count() * 1.5)
        executor = concurrent.futures.ThreadPoolExecutor(thread_pool_size)

        task = loop.create_task(_run_server(executor, args))

        def signal_handler(signal_name):
            logger.warning("Received %s, cancelling the main task", signal_name)
            task.cancel()

        loop.add_signal_handler(signal.SIGINT, signal_handler, "SIGINT")
        loop.add_signal_handler(signal.SIGTERM, signal_handler, "SIGTERM")

    except docopt.DocoptExit:
        print(__doc__.strip())
        return 2
    except Exception:
        logger.exception("Exception raised before the main loop started")
        return 1

    try:
        loop.run_until_complete(task)
    except CancelledError:
        logger.warning("Main task was canceled")
    except Exception:
        logger.exception("Exception raised from the main task")
        return 1
    finally:
        task.cancel()
        while not task.done():
            try:
                loop.run_until_complete(task)
            except Exception:
                logger.exception("Exception raised from the finishing main task")
        if executor is not None:
            logger.debug("Shutting down the executor")
            executor.shutdown()

    logger.debug("Bye")
    return 0

if __name__ == "__main__":
    args = docopt.docopt(__doc__, argv = sys.argv[1:], help = False)
    _main(args)

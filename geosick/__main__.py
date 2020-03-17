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
import concurrent
import docopt
import logging
import sys
import signal
import os
import sys
import time

from asyncio.futures import CancelledError
from geosick.server import run_server

logger = logging.getLogger("geosick.main")

def _main():
    _init_logging()

    try:
        args = docopt.docopt(__doc__, argv = sys.argv[1:], help = False)
        loop = asyncio.get_event_loop()

        thread_pool_size = int(os.cpu_count() * 1.5)
        executor = concurrent.futures.ThreadPoolExecutor(thread_pool_size)

        task = loop.create_task(run_server(executor, args))

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

def _init_logging():
    formatter = logging.Formatter(
        fmt = "%(asctime)s %(levelname)s %(name)s: %(message)s",
        datefmt = "%Y-%m-%d %H:%M:%S")
    formatter.converter = time.gmtime
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)

    for logger_name in ("geosick",):
        logger = logging.getLogger(logger_name)
        if "GEOSICK_DEBUG" in os.environ:
            logger.setLevel(logging.DEBUG)
        else:
            logger.setLevel(logging.INFO)
        logger.addHandler(handler)

    asyncio_logger = logging.getLogger("asyncio")
    asyncio_logger.addHandler(handler)

    if "GEOSICK_ASYNCIO_DEBUG" in os.environ:
        asyncio_logger.setLevel(logging.DEBUG)
    else:
        asyncio_logger.setLevel(logging.WARNING)

if __name__ == "__main__":
    sys.exit(_main())

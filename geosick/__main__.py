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

import docopt
import sys


def _main():
    args = docopt.docopt(__doc__, argv = sys.argv[1:], help = False)
    port = int(args.get("--port") or 4100)

if __name__ == "__main__":
    sys.exit(_main())

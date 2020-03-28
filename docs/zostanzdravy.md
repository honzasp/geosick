# Geosick for ZostanZdravy

The geosick program for ZostanZdravy loads stored user positions from a MySQL
database, finds potentially risky contacts between infected and healthy users,
and stores the matches into a JSON file.

## Build

The project is built using Meson, executable `zostanzdravy`. Run

    meson setup build

to setup a build directory named `build`, and then run

    ninja -C build

to start the build process. The resulting executable will be stored in
`build/zostanzdravy`.

There is also `Dockerfile.zostanzdravy`, which builds a Docker image with the
program.

## Usage

The program takes a single argument, which is a path to the config file:

    ./zostanzdravy <config-file>

The config file is a JSON with the following structure:

    {
        "mysql": {
            "db": "<MySQL database name>",
            "host": "<MySQL server hostname>",
            "port": <MySQL server port>,
            "user": "<MySQL user name>",
            "password": "<MySQL user password>"
        },
        "range_days": <number of days in the past that are considered>,
        "period_s": <sampling period of the algorithm in seconds>,
        "temp_dir": "<path to a directory for storing temporary files>",
        "row_buffer_size": <maximum number of rows that will be stored in memory>
    }


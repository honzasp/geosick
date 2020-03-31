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

The config file is a JSON with the following fields:

- `mysql.db`: name of the MySQL database.
- `mysql.server`: MySQL server as "/path/to/socket" or "host:port".
- `mysql.user`: MySQL user name.
- `mysql.password`: MySQL user password.
- `mysql.ssl_mode`: MySQL SSL mode ("DISABLED", "PREFERRED", "REQUIRED", default
    "PREFERRED").
- `sange_days`: Number of days in the past that are considered for the matches
    (default 14).
- `period_s`: Sampling period of the algorithm in seconds (default 30).
- `temp_dir`: Path to a directory for storing temporary files.
- `row_buffer_size`: Size of the buffer that stores rows in memory before
    dumping them to disk (default 4000000).

(Dots in the field names represent nested objects.)

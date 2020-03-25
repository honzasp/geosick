## Batch processing

Our batch processing tool is meant to be used for identifying users that have a high chance of
being infected by already sick people whose GPS location history is known. The tool is written
mostly in C++ and is supposed to be able to process databases of hundreds of thousands of users.
Here follows the description of how it works internally.

# Data load

The first step is to load all of the data from the database. In the current setup it's written to load
the data from MySQL database, but almost any other DB can be used (even the NoSQL ones like ScyllaDB).
We expect the data to be in a single table with columns
`user_id, timestamp_utc_s, latitude, longitude, accuracy_m, altitude_m, heading_deg, speed_mps`.

To cope with large data volumes without the need for a large RAM, we stream the data into multiple
files that are stored on a disk. Before each file is saved, we sort its content based on UserID
and timestamp.

# Sampling

To make further data processing easier, we first perform something we call sampling. It is a process
during which we take GPS sequence from each user individually and sample it so that we get a new GPS
sequence that has a uniform time period. In other words, every P seconds, we get a GPS point for
all users (or nothing if it's unavailable). To do that we need to separate GPS sequence for each user
and sort it based on a timestamp. To do that we form a buffered heap on top of the files we
generated in the previous step.

# Prefiltering

The final probability evaluation process is computationally quite heavy and hence we try avoiding
computing it for all user-sick pairs. To do that we roughly prefilter the user-sick pairs. The
prefiltering is done by creating a fast search structure from GPS history of all of the sick people
and then iterating over all of our users and asking whether there is an intersection of their
GPS-time data with anyone from the infected group. The search structure is currently based
on hashing of (GPS, time) tuple.

# Final evaluation

The final step is to take all of the "promising" pairs from the previous step and run it through
our [HTTP service](http_server.md). User pairs that show significantly high probability of
sickness transmission are then stored to another MySQL table.

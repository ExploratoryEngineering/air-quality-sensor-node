# Introduction

The Air Quality Server (AQS) is a lightweight tool that can be used to
receive, store and serve air quality data from the third generation
air quality sensor from Exploratory Engineering.  It can both be run
as a more permanent server in a datacenter in order to receive,
process, store and pass on data, but it is also designed for ad-hoc
experimentation by researchers and integrators.  Some care has been
taken in order to make both scenarios possible.  It should be trivial
to download, build and run the server on your personal machine, as
well as running it in a datacenter.

## Data reception

Currently the server supports two different ways of receiving data -
you can either configure it to connect to IoT-GW/Horde and have it
listen to the data stream belonging to a given *collection*.

We are also in the process of developing a solution that will allow
the server to use MIC as the source of messages, but this is not
supported at the time of writing.

For experimentation purposes it is also possible to use a UDP-based
protocol for injecting messages into a running server.

## Database storage

The AQS makes use of a local SQLite 3 database where it stores
calibration data as well as the messages (datapoints) it has received
from the network.  We chose to use SQLite 3 as the database as a
convenience because the data rates and amounts of data are fairly
modest for the number of sensors we are dealing with.  At some later
point we may support PostgreSQL so that the Air Quality Server can
deal with higher volumes.

The choice of embedding the database was made to make life simpler for
researchers and people who simply want to experiment with the system,
as there is no need to set up and manage a separate database system.
Everything is taken care of by the server.  The database itself is
located in a single file, which makes it easy to copy the data around
and perform ad-hoc queries on it using the `sqlite3` command line
utility.

## Open source

The AQS is published under the Apache 2.0 license, and you are
encouraged to contribute to its development.  Bug reports and feature
requests filed through the issue tracking on the Github project are
welcome.  Pull requests with code are even more welcome. The Git repository is located at
[https://github.com/exploratoryengineering/air-quality-sensor-node](https://github.com/exploratoryengineering/air-quality-sensor-node).

\newpage

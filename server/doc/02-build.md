# Building AQS

## Check out source from Github

The AQS is hosted on Github at and you can clone it by issuing the
command:

    git clone https://github.com/ExploratoryEngineering/air-quality-sensor-node.git

This will copy not only the server, but also the hardware designs for
the sensor, the firmware source code for the device, and the server.
In order to build the server change directory to the `server`
directory.

## Fetch dependencies

  - Go 1.14 or newer
  - Protobuf Compiler version 3.0.0 or newer
  - Protobuf Code generator for Go
  - SQLite3 libraries

### Installing Go

For information on how to install Go, please refer to
https://golang.org/doc/install.


### Installing Protobuf

To install the protobuf compiler on macOS the easiest route is to use
Homebrew and install it with:

    brew install protobuf
	
or on Linux

    apt install protobuf-compiler

You then need to install the protobuf code generator for Go, which you
can install with the command, which should work on all platforms Go
has been ported to:

    go install google.golang.org/protobuf/cmd/protoc-gen-go

### Installing libsqlite3

On macOS you can install the latest SQLite version (including
libraries) using

    brew install sqlite

On Linux you can do this with

    apt install libsqlite3-dev


## Building AQS with `make`

We use `make` to build the server.  Per default the `Makefile` is set
up to build the server for macOS.  You can either edit the `Makefile`
and change the default values for `GOOS` and `GOARCH` to fit the
platform you are building on, or you can specify these variables on
the command line.

For instance in order to build for Linux you would 

    GOOS=linux GOARCH=amd64 make

This should produce a `bin` directory containing a runnable binary
called `ac`.  To test that you have succeeded in building it you can
run `bin/aq` and it should produce output that looks something like
this:

    $ bin/aq -h
    Usage:
      aq [OPTIONS] <command>
    
    Application Options:
      -c, --collection=<collectionID>    Horde collection id to listen to (default: 17dh0cf43jg007)
      -d, --db=<file>                    Data storage file (default: aq.db)
      -v, --verbose
    
    Help Options:
      -h, --help                         Show this help message
    
    Available commands:
      convert  Convert calibration data
      fetch    Fetch historical data
      import   Import a calibration data
      list     List calibration data
      rm       Remove calibration data
      run      Run server
      show     Show calibration data for device
    
\newpage

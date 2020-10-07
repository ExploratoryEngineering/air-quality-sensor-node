# Trondheim Kommune Air Quality Server

The Air Quality Server provides a simple tool for receiving real time
data from the Trondheim Kommune Air Quality Sensors.  It supports
fetching 

## Installing

You need to install the `protoc` protocol compiler for Google
protobuffers. 

On OSX:

    brew install protobuf

On Linux:

    sudo apt install protobuf-compiler

## Building

When you build for the first time you need to make sure you have
`github.com/golang/protobuf/protoc-gen-go` installed.  You can install
this by issuing the following command

    make dep_install

The you can simply run

    make
	
This should build the AQ server.  The binary will be in `bin/aq`.  We
have not provided an install target in the Makefile since we cannot
really assume anything about where you would want it installed. One
recommendation is to place the `aq` binary in `/usr/local/bin/aq`, but
that is just a suggestion.

## Running AQ

In order to run AQ you will need an API key and an API endpoint.

**This is currently subject to change so documentation for this will
turn up when we have things landed.  This shouldn't take more than at
most a few weeks. **



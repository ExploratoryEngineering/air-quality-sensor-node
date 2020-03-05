package main

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/udplistener"
)

func main() {
	opts := opts.Parse()

	// Start UDP Listener
	udpListener := udplistener.New(opts)
	udpListener.Start()
}

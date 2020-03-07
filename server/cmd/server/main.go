package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/udplistener"
)

func main() {
	opts := opts.Parse()

	// Start UDP Listener
	log.Printf("Starting UDP listener on %s", opts.UDPListenAddress)
	udpListener := udplistener.New(opts)
	udpListener.Start()
	udpListener.WaitForShutdown()
}

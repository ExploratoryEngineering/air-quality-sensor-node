package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
)

var listeners []listener.Listener

func main() {
	opts := opts.Parse()

	// Set up pipeline
	pipelineRoot := pipeline.NewRoot(opts)
	pipelineLog := pipeline.NewLog(opts)
	pipelinePersist := pipeline.NewPersist(opts)

	pipelineRoot.AddNext(pipelineLog)
	pipelineLog.AddNext(pipelinePersist)

	// Start Horde listener unless disabled
	if !opts.HordeListenerDisable {
		log.Printf("Starting Horde listener.  Listening to collection='%s'", opts.HordeCollection)
		hordeListener := listener.NewHordeListener(opts, pipelineRoot)
		err := hordeListener.Start()
		if err != nil {
			log.Fatal("Unable to start Horde listener: %v", err)
		}
		listeners = append(listeners, hordeListener)

	}

	// Start UDP listener if configured
	if opts.UDPListenAddress != "" {
		// Start UDP Listener
		log.Printf("Starting UDP listener on %s", opts.UDPListenAddress)
		udpListener := listener.NewUDPListener(opts, pipelineRoot)
		err := udpListener.Start()
		if err != nil {
			log.Fatal("Unable to start UDP listener: %v", err)
		}
		listeners = append(listeners, udpListener)
	}

	// Wait for all listeners to shut down
	for _, listener := range listeners {
		listener.WaitForShutdown()
	}
}
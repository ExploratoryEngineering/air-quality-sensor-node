package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

var listeners []listener.Listener

func main() {
	opts := opts.Parse()

	// Set up persistence
	db, err := sqlitestore.New(opts.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", opts.DBFilename, err)
	}

	// Set up pipeline
	pipelineRoot := pipeline.NewRoot(opts)
	pipelineCalc := pipeline.NewCalculate(opts, db)
	pipelinePersist := pipeline.NewPersist(opts, db)
	pipelineLog := pipeline.NewLog(opts)

	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)
	pipelinePersist.AddNext(pipelineLog)

	// Start Horde listener unless disabled
	if !opts.HordeListenerDisable {
		log.Printf("Starting Horde listener.  Listening to collection='%s'", opts.HordeCollection)
		hordeListener := listener.NewHordeListener(opts, pipelineRoot)
		err := hordeListener.Start()
		if err != nil {
			log.Fatalf("Unable to start Horde listener: %v", err)
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
			log.Fatalf("Unable to start UDP listener: %v", err)
		}
		listeners = append(listeners, udpListener)
	}

	// Wait for all listeners to shut down
	for _, listener := range listeners {
		listener.WaitForShutdown()
	}
}

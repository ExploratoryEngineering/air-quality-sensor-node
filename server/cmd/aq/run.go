package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// RunCommand ...
type RunCommand struct {
	// Webserver options
	WebServer string `short:"w" long:"webserver" description:"Listen address for webserver" default:":8888" value-name:"<[host]:port>"`

	// Horde listener
	HordeListenerDisable bool `short:"x" long:"no-horde" description:"Do not connect to Horde"`

	// UDP listener
	UDPListenAddress string `short:"u" long:"udp-listener" description:"Listen address for UDP listener" default:"" value-name:"<[host]:port>"`
	UDPBufferSize    int    `short:"b" long:"udp-buffer-size" description:"Size of UDP read buffer" default:"1024" value-name:"<num bytes>"`
}

func init() {
	parser.AddCommand(
		"run",
		"Run server",
		"Run server",
		&RunCommand{})
}

var listeners []listener.Listener

// Execute ...
func (a *RunCommand) Execute(args []string) error {
	// Set up persistence
	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", options.DBFilename, err)
	}

	// Set up pipeline
	pipelineRoot := pipeline.NewRoot(&options, db)
	pipelineCalc := pipeline.NewCalculate(&options, db)
	pipelinePersist := pipeline.NewPersist(&options, db)
	pipelineLog := pipeline.NewLog(&options)

	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)
	pipelinePersist.AddNext(pipelineLog)

	// Start Horde listener unless disabled
	if !a.HordeListenerDisable {
		log.Printf("Starting Horde listener.  Listening to collection='%s'", options.HordeCollection)
		hordeListener := listener.NewHordeListener(&options, pipelineRoot)
		err := hordeListener.Start()
		if err != nil {
			log.Fatalf("Unable to start Horde listener: %v", err)
		}
		listeners = append(listeners, hordeListener)

	}

	// Start UDP listener if configured
	if a.UDPListenAddress != "" {
		// Start UDP Listener
		log.Printf("Starting UDP listener on %s", a.UDPListenAddress)
		udpListener := listener.NewUDPListener(a.UDPListenAddress, a.UDPBufferSize, pipelineRoot)
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

	return nil
}

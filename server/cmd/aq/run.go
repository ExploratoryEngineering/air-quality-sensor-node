package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/api"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener/hordelistener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener/udplistener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/calculate"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/persist"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipelog"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipemqtt"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

const (
	circularBufferLength = 100
)

// RunCommand ...
type RunCommand struct {
	// Webserver options
	WebListenAddr   string `short:"w" long:"web-listen-address" description:"Listen address for webserver" default:":8888" value-name:"<[host]:port>"`
	WebStaticDir    string `short:"s" long:"web-static-dir" description:"Static directory for webserver" default:"./web" value-name:"<dir>"`
	WebTemplateDir  string `short:"t" long:"web-template-dir" description:"Template directory for webserver" default:"./templates" value-name:"<dir>"`
	WebAccessLogDir string `short:"l" long:"web-access-log-dir" description:"Directory for access logs" default:"./logs" value-name:"<dir>"`

	// MQTT
	MQTTAddress     string `long:"mqtt-address" description:"MQTT Address" default:"" value-name:"<[host]:port>"`
	MQTTClientID    string `long:"mqtt-client-id" env:"MQTT_CLIENT_ID" description:"MQTT Client ID" default:""`
	MQTTPassword    string `long:"mqtt-password" env:"MQTT_PASSWORD" description:"MQTT Password" default:""`
	MQTTTopicPrefix string `long:"mqtt-topic-prefix" description:"MQTT topic prefix" default:"aq" value-name:"MQTT topic prefix"`

	// Horde listener
	HordeListenerEnable bool `long:"enable-horde" description:"Connect to Horde"`

	// UDP listener
	UDPListenAddress string `long:"udp-listener" description:"Listen address for UDP listener" default:"" value-name:"<[host]:port>"`
	UDPBufferSize    int    `long:"udp-buffer-size" description:"Size of UDP read buffer" default:"1024" value-name:"<num bytes>"`
}

func init() {
	parser.AddCommand(
		"run",
		"Run server",
		"Run server",
		&RunCommand{})
}

var listeners []listener.Listener

// startUDPListener starts the UDP listener
func (a *RunCommand) startUDPListener(r pipeline.Pipeline) listener.Listener {
	log.Printf("Starting UDP listener on %s", a.UDPListenAddress)
	udpListener := udplistener.New(a.UDPListenAddress, a.UDPBufferSize, r)
	err := udpListener.Start()
	if err != nil {
		log.Fatalf("Unable to start UDP listener: %v", err)
	}
	listeners = append(listeners, udpListener)
	return udpListener
}

// startHordeListener
func (a *RunCommand) startHordeListener(r pipeline.Pipeline) listener.Listener {
	log.Printf("Starting Horde listener.  Listening to collection='%s'", options.HordeCollection)
	hordeListener := hordelistener.New(&options, r)
	err := hordeListener.Start()
	if err != nil {
		log.Fatalf("Unable to start Horde listener: %v", err)
	}
	listeners = append(listeners, hordeListener)
	return hordeListener
}

// Execute ...
func (a *RunCommand) Execute(args []string) error {
	// Set up persistence
	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", options.DBFilename, err)
	}
	defer db.Close()

	// Load the calibration data to pick up any new calibration sets
	loadCalibrationData(db, options.CalibrationDataDir)

	// Create pipeline elements
	// TODO(borud): make streaming broker configurable
	pipelineRoot := pipeline.New(&options, db)
	pipelineCalc := calculate.New(&options, db)
	pipelinePersist := persist.New(&options, db)
	pipelineLog := pipelog.New(&options)
	pipelineCirc := circular.New(circularBufferLength)
	pipelineStream := stream.NewBroker()

	// Chain them together
	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)
	pipelinePersist.AddNext(pipelineLog)
	pipelineLog.AddNext(pipelineStream)
	pipelineStream.AddNext(pipelineCirc)

	// Stream to MQTT server if enabled
	if a.MQTTAddress != "" {
		pipelineMQTT := pipemqtt.New(a.MQTTClientID, a.MQTTPassword, a.MQTTAddress, a.MQTTTopicPrefix)
		pipelineCirc.AddNext(pipelineMQTT)
	}

	// Start Horde listener if enabled
	a.startHordeListener(pipelineRoot)

	// Start UDP listener if configured
	if a.UDPListenAddress != "" {
		a.startUDPListener(pipelineRoot)
	}

	// If we have no listeners there is no point to starting so we terminate
	if len(listeners) == 0 {
		log.Fatalf("No listeners defined so terminating.  Please specify at least one listener.")
	}

	// Start api server
	api := api.New(&api.ServerConfig{
		Broker:         pipelineStream,
		DB:             db,
		CircularBuffer: pipelineCirc,
		ListenAddr:     a.WebListenAddr,
		StaticDir:      a.WebStaticDir,
		TemplateDir:    a.WebTemplateDir,
		AccessLogDir:   a.WebAccessLogDir,
	})
	api.Start()

	// Wait for all listeners to shut down
	for _, listener := range listeners {
		listener.WaitForShutdown()
	}
	api.Shutdown()
	return nil
}

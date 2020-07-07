package main

import (
	"log"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/api"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/calculate"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/persist"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipelog"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipemqtt"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// How often do we poll for new calibration data
const (
	checkForNewCalibrationDataPeriod = (1 * time.Hour)
	circularBufferLength             = 100
)

// RunCommand ...
type RunCommand struct {
	// Webserver options
	WebListenAddr   string `short:"w" long:"web-listen-address" description:"Listen address for webserver" default:":8888" value-name:"<[host]:port>"`
	WebStaticDir    string `short:"s" long:"web-static-dir" description:"Static directory for webserver" default:"./web" value-name:"<dir>"`
	WebTemplateDir  string `short:"t" long:"web-template-dir" description:"Template directory for webserver" default:"./templates" value-name:"<dir>"`
	WebAccessLogDir string `short:"l" long:"web-access-log-dir" description:"Directory for access logs" default:"./logs" value-name:"<dir>"`

	// MQTT
	MQTTAddress     string `short:"m" long:"mqtt-address" description:"MQTT Address" default:"" value-name:"<[host]:port>"`
	MQTTClientID    string `short:"c" long:"mqtt-client-id" description:"MQTT Client ID" default:"id"`
	MQTTPassword    string `short:"p" long:"mqtt-password" description:"MQTT Password" default:""`
	MQTTTopicPrefix string `long:"mqtt-topic-prefix" description:"MQTT topic prefix" default:"aq" value-name:"MQTT topic prefix"`

	// Horde listener
	HordeListenerDisable bool `short:"x" long:"no-horde" description:"Do not connect to Horde"`

	// UDP listener
	UDPListenAddress string `short:"u" long:"udp-listener" description:"Listen address for UDP listener" default:"" value-name:"<[host]:port>"`
	UDPBufferSize    int    `short:"b" long:"udp-buffer-size" description:"Size of UDP read buffer" default:"1024" value-name:"<num bytes>"`

	// Turn off downloading of calibration data
	NoCalDownload bool `short:"n" long:"no-cal-download" description:"Turn off download of calibration data"`
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
	defer db.Close()

	if !a.NoCalDownload {
		// Check for new calibration data
		log.Printf("Checking for new calibration data (disable with -n option)")
		err = checkForNewCalibrationData(db)
		if err != nil {
			log.Printf("Unable to check for calibration data: %v", err)
			log.Printf("Will continue with possibly stale calibration data")
		}

		// Start periodic download of new calibration data
		go periodicCheckForNewCalibrationData(db)
	}

	// Create pipeline elements
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

	if a.MQTTAddress != "" {
		pipelineMQTT := pipemqtt.New(a.MQTTClientID, a.MQTTPassword, a.MQTTAddress, a.MQTTTopicPrefix)
		pipelineCirc.AddNext(pipelineMQTT)
	}

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

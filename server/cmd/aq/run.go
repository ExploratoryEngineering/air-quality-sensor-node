package main

import (
	"log"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/api"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener/hordelistener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener/miclistener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener/udplistener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/calculate"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/persist"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipelog"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipemqtt"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
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
	MQTTAddress     string `long:"mqtt-address" description:"MQTT Address" default:"" value-name:"<[host]:port>"`
	MQTTClientID    string `long:"mqtt-client-id" env:"MQTT_CLIENT_ID" description:"MQTT Client ID" default:""`
	MQTTPassword    string `long:"mqtt-password" env:"MQTT_PASSWORD" description:"MQTT Password" default:""`
	MQTTTopicPrefix string `long:"mqtt-topic-prefix" description:"MQTT topic prefix" default:"aq" value-name:"MQTT topic prefix"`

	// MIC
	MICListenerEnabled bool   `long:"enable-mic" description:"Connect to MIC"`
	MICUsername        string `long:"mic-username" env:"MIC_USERNAME" description:"MIC Username" default:""`
	MICPassword        string `long:"mic-password" env:"MIC_PASSWORD" description:"MIC Username" default:""`
	MICAWSAPIKey       string `long:"mic-api-key" env:"MIC_AWS_API_KEY" description:"MIC Username" default:""`
	MICTopic           string `long:"mic-topic" description:"MIC topic we should listen to" default:"thing-update/StartIoT/trondheim.kommune.no/#"`
	MICAWSRegion       string `long:"mic-aws-region" description:"AWS region for MIC" default:"eu-west-1"`
	MICAWSAPIGateway   string `long:"mic-aws-api-gw" description:"AWS API gateway" default:"https://3ohe8pnzfb.execute-api.eu-west-1.amazonaws.com/prod"`
	MICAWSUserPool     string `long:"mic-aws-user-pool" description:"AWS User pool" default:"eu-west-1_wsOo2av1M"`
	MICAWSIoTEndpoint  string `long:"mic-aws-iot-endpoint" desciption:"AWS IoT Endpoint" default:"a15nxxwvsld4o-ats"`

	// Horde listener
	HordeListenerEnable bool `long:"enable-horde" description:"Connect to Horde"`

	// UDP listener
	UDPListenAddress string `long:"udp-listener" description:"Listen address for UDP listener" default:"" value-name:"<[host]:port>"`
	UDPBufferSize    int    `long:"udp-buffer-size" description:"Size of UDP read buffer" default:"1024" value-name:"<num bytes>"`

	// Skip initial download of calibration data
	SkipInitialCalDownload bool `short:"n" long:"skip-cal-download" description:"Turn off initial download of calibration data"`
}

func init() {
	parser.AddCommand(
		"run",
		"Run server",
		"Run server",
		&RunCommand{})
}

var listeners []listener.Listener

func (a *RunCommand) downloadCalibrationData(db store.Store) {
	// Check for new calibration data
	log.Printf("Checking for new calibration data (disable with -n option)")
	err := checkForNewCalibrationData(db)
	if err != nil {
		log.Printf("Unable to check for calibration data: %v", err)
		log.Printf("Will continue with possibly stale calibration data")
	}
}

// startMICListener starts the MIC listener
func (a *RunCommand) startMICListener(r pipeline.Pipeline) listener.Listener {
	log.Printf("Starting MIC listener. endpoint='%s' topic='%s'", a.MICAWSIoTEndpoint, a.MICTopic)
	micListener := miclistener.New(&options, r, &miclistener.Config{
		Username:       a.MICUsername,
		Password:       a.MICPassword,
		AWSAPIKey:      a.MICAWSAPIKey,
		Topic:          a.MICTopic,
		AWSRegion:      a.MICAWSRegion,
		AWSAPIGateway:  a.MICAWSAPIGateway,
		AWSUserPool:    a.MICAWSUserPool,
		AWSIoTEndpoint: a.MICAWSIoTEndpoint,
	})
	err := micListener.Start()
	if err != nil {
		log.Fatalf("Unable to start UDP listener: %v", err)
	}
	listeners = append(listeners, micListener)
	return micListener
}

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

	// Download calibration data.
	if !a.SkipInitialCalDownload {
		a.downloadCalibrationData(db)
	}

	// Start periodic download of new calibration data
	go periodicCheckForNewCalibrationData(db)

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

	// Start MIC listener if enabled
	if a.MICListenerEnabled {
		a.startMICListener(pipelineRoot)
	}

	// Start Horde listener if enabled
	if a.HordeListenerEnable {
		a.startHordeListener(pipelineRoot)
	}

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

package main

import (
	"log"
	"strings"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/api"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/caldata"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/listener"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// How often do we poll for new calibration data
const (
	checkForNewCalibrationDataPeriod = (12 * time.Hour)
	circularBufferLength             = 100
)

// RunCommand ...
type RunCommand struct {
	// Webserver options
	WebListenAddr  string `short:"w" long:"web-listen-address" description:"Listen address for webserver" default:":8888" value-name:"<[host]:port>"`
	WebStaticDir   string `short:"s" long:"web-static-dir" description:"Static directory for webserver" default:"./web" value-name:"<dir>"`
	WebTemplateDir string `short:"t" long:"web-template-dir" description:"Template directory for webserver" default:"./templates" value-name:"<dir>"`

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

// checkForNewCalibrationData fetches the calibration data from the
// distribution point in S3 and attempts to insert it into the
// database.  If it already exists the database will just reject it
// with a constraint error.
func checkForNewCalibrationData(db store.Store) error {
	c, err := caldata.NewCaldata(options.CalDataS3URL)
	if err != nil {
		return err
	}

	// Get all the calibration data available from the S3 distribution point
	cals, err := c.DownloadFromS3()
	if err != nil {
		return err
	}

	// Loop through it and see if we have calibration data for the device
	for _, cal := range cals {
		// Just try to insert it and see if it succeeds, if we get a
		// constraint error we know the calibration data is there
		// already
		_, err := db.PutCal(&cal)
		if err != nil {
			if !strings.Contains(err.Error(), "UNIQUE constraint failed") {
				log.Printf("Error inserting calibration data: %+v", err)
			}
		}
	}

	return nil
}

// periodicCheckForNewCalibrationData sleeps for
// checkForNewCalibrationDataPeriod and then downloads calibration
// data and attempts to insert it.
func periodicCheckForNewCalibrationData(db store.Store) {
	for {
		time.Sleep(checkForNewCalibrationDataPeriod)
		log.Printf("Checking for new calibration data")
		err := checkForNewCalibrationData(db)
		if err != nil {
			log.Printf("Failed to check for new calibration data: %v", err)
		}
	}
}

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
		err = checkForNewCalibrationData(db)
		if err != nil {
			log.Printf("Unable to check for calibration data: %v", err)
			log.Printf("Will continue with possibly stale calibration data")
		}

		// Start periodic download of new calibration data
		go periodicCheckForNewCalibrationData(db)
	}

	// Set up pipeline
	pipelineRoot := pipeline.NewRoot(&options, db)
	pipelineCalc := pipeline.NewCalculate(&options, db)
	pipelinePersist := pipeline.NewPersist(&options, db)
	pipelineLog := pipeline.NewLog(&options)
	pipelineCirc := pipeline.NewCircularBuffer(circularBufferLength)
	pipelineStream := stream.NewBroker()

	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)
	pipelinePersist.AddNext(pipelineLog)
	pipelineLog.AddNext(pipelineStream)
	pipelineStream.AddNext(pipelineCirc)

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
	})
	api.Start()

	// Wait for all listeners to shut down
	for _, listener := range listeners {
		listener.WaitForShutdown()
	}

	api.Shutdown()

	return nil
}

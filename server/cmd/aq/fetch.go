package main

import (
	"log"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
	"github.com/telenordigital/nbiot-go"
)

// FetchCommand ...
type FetchCommand struct {
}

var fetchCommand FetchCommand

const pageSize = 100

// For this application we say that time begins at this date.
var beginningOfTime = time.Date(2020, 3, 25, 0, 0, 0, 0, time.UTC)

func init() {
	parser.AddCommand("fetch", "Fetch historical data", "Fetch historical sensor data from Horde server", &fetchCommand)
}

// Execute ...
func (a *FetchCommand) Execute(args []string) error {
	client, err := nbiot.New()
	if err != nil {
		return err
	}

	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", options.DBFilename, err)
	}

	// Set up pipeline
	pipelineRoot := pipeline.NewRoot(&options, db)
	pipelineCalc := pipeline.NewCalculate(&options, db)
	pipelinePersist := pipeline.NewPersist(&options, db)

	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)

	var since = beginningOfTime
	var until time.Time
	var count = 0
	var countTotal = 0
	for {
		data, err := client.CollectionData(options.HordeCollection, since, until, pageSize)
		if err != nil {
			log.Fatalf("Error while reading data: %v", err)
		}

		if len(data) == 0 {
			return nil
		}

		for _, d := range data {
			pb, err := model.ProtobufFromData(d.Payload)
			if err != nil {
				log.Printf("Failed to decode protobuffer len=%d: %v", len(d.Payload), err)
				continue
			}

			m := model.MessageFromProtobuf(pb)
			if m == nil {
				log.Printf("Unable to create Message from protobuf")
				continue
			}

			m.DeviceID = d.Device.ID
			m.ReceivedTime = time.Unix(d.Received/1000, 0)
			m.PacketSize = len(d.Payload)

			pipelineRoot.Publish(m)

			count++
			countTotal++
		}

		until = msToTime(data[len(data)-1].Received)

		if count >= 500 {
			log.Printf("Imported %d records...", countTotal)
			count = 0
		}
	}
}

// msToTime converts milliseconds since epoch to time.Time
func msToTime(t int64) time.Time {
	return time.Unix(t/int64(1000), (t%int64(1000))*int64(1000000))
}
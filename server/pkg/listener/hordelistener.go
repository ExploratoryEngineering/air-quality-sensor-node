package listener

import (
	"errors"
	"io"
	"log"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/telenordigital/nbiot-go"
)

// ErrNoHordeCollection indicates that the Horde collectionID is an empty string
var ErrNoHordeCollection = errors.New("No Horde CollectionID specified, please set -c or --collection command line option")

// HordeListener connects to Horde and listens for messages from a
// particular Collection
type HordeListener struct {
	pipeline     pipeline.Pipeline
	collectionID string
	doneChan     chan error
	quit         chan bool
	client       *nbiot.Client
}

// NewHordeListener creates a new HordeListener instance
func NewHordeListener(opts *opts.Opts, pipeline pipeline.Pipeline) *HordeListener {
	return &HordeListener{
		pipeline:     pipeline,
		collectionID: opts.HordeCollection,
		doneChan:     make(chan error),
		quit:         make(chan bool),
	}
}

// Start HordeListener instance
func (h *HordeListener) Start() error {
	if h.collectionID == "" {
		return ErrNoHordeCollection
	}

	c, err := nbiot.New()
	if err != nil {
		return err
	}
	h.client = c

	stream, err := h.client.CollectionOutputStream(h.collectionID)
	if err != nil {
		log.Fatal("Error creating stream: ", err)
	}

	go func() {
		defer stream.Close()
		for {
			data, err := stream.Recv()
			if err == io.EOF {
				h.doneChan <- err
				break
			}
			if err != nil {
				h.doneChan <- err
			}

			pb, err := model.ProtobufFromData(data.Payload)
			if err != nil {
				log.Printf("Failed to decode protobuffer len=%d: %v", len(data.Payload), err)
				continue
			}

			dp := model.DataPointFromProtobuf(pb)
			if dp == nil {
				log.Printf("Unable to create DataPoint from protobuf")
				continue
			}

			h.pipeline.Publish(&model.Message{
				DeviceID:     data.Device.ID,
				CollectionID: data.Device.CollectionID,
				ReceivedTime: time.Unix(data.Received/1000, 0),
				PacketSize:   len(data.Payload),
				DataPoint:    dp,
			})
		}
	}()

	return nil
}

// Shutdown initiates shutdown of the UDPListener
func (h *HordeListener) Shutdown() {
	log.Printf("UDPListener: Shutdown not implemented")
}

// WaitForShutdown waits for the UDP listener to shut down
func (h *HordeListener) WaitForShutdown() {
	<-h.quit
}

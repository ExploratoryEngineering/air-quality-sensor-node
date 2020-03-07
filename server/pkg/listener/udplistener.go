package listener

import (
	"context"
	"fmt"
	"log"
	"net"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
)

// UDPListener listens for UDP packets on a given listenAddress
type UDPListener struct {
	listenAddress string
	bufferSize    int
	ctx           context.Context
	pipeline      pipeline.Pipeline
	doneChan      chan error
	quit          chan bool
}

// NewUDPListener creates a new UDPListener instance
func NewUDPListener(opts *opts.Opts, pipeline pipeline.Pipeline) *UDPListener {
	return &UDPListener{
		listenAddress: opts.UDPListenAddress,
		bufferSize:    opts.UDPBufferSize,
		ctx:           context.Background(),
		pipeline:      pipeline,
		doneChan:      make(chan error),
		quit:          make(chan bool),
	}
}

// Start UDPListener instance
func (u *UDPListener) Start() error {
	pc, err := net.ListenPacket("udp", u.listenAddress)
	if err != nil {
		return fmt.Errorf("Failed to listen to %s: %v", u.listenAddress, err)
	}

	buffer := make([]byte, u.bufferSize)
	go func() {
		defer pc.Close()
		for {
			n, addr, err := pc.ReadFrom(buffer)
			if err != nil {
				u.doneChan <- err
				return
			}

			pb, err := model.ProtobufFromData(buffer[:n])
			if err != nil {
				log.Printf("Error decoding packet from %v into pb: %v", addr, err)
				continue
			}

			dp := model.DataPointFromProtobuf(pb)

			u.pipeline.Publish(&model.Message{
				ReceivedTime: time.Now(),
				PacketSize:   n,
				DataPoint:    dp,
			})

		}
	}()

	return nil
}

// Shutdown initiates shutdown of the UDPListener
func (u *UDPListener) Shutdown() {
	log.Printf("UDPListener: Shutdown not implemented")
}

// WaitForShutdown waits for the UDP listener to shut down
func (u *UDPListener) WaitForShutdown() {
	<-u.quit
}

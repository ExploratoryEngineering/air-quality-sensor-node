package udplistener

import (
	"context"
	"fmt"
	"log"
	"net"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
)

// UDPListener listens for UDP packets on a given listenAddress
type UDPListener struct {
	listenAddress string
	bufferSize    int
	ctx           context.Context
	doneChan      chan error
	quit          chan bool
}

// New creates a new UDPListener instance
func New(opts *opts.Opts) *UDPListener {
	return &UDPListener{
		listenAddress: opts.UDPListenAddress,
		bufferSize:    opts.UDPBufferSize,
		doneChan:      make(chan error),
		quit:          make(chan bool),
		ctx:           context.Background(),
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
			log.Printf("packet-received: bytes=%d from=%s\n", n, addr.String())
		}
	}()

	return nil
}

// WaitForShutdown waits for the UDP listener to shut down
func (u *UDPListener) WaitForShutdown() {
	<-u.quit
}

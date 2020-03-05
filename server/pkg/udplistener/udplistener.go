package udplistener

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
)

// UDPListener listens for UDP packets on a given listenAddress
type UDPListener struct {
	listenAddress string
}

// New creates a new UDPListener instance
func New(opts *opts.Opts) *UDPListener {
	return &UDPListener{
		listenAddress: opts.UDPListenAddress,
	}
}

// Start UDPListener instance
func (u *UDPListener) Start() error {
	return nil
}

// Stop UDPListener instance
func (u *UDPListener) Stop() error {
	return nil
}

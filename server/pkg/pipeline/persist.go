package pipeline

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
)

// Persist is a pipeline processor that persists incoming messages
type Persist struct {
	next Pipeline
	opts *opts.Opts
}

// NewPersist creates new Persist pipeline element
func NewPersist(opts *opts.Opts) *Persist {
	return &Persist{
		opts: opts,
	}
}

// Publish ...
func (p *Persist) Publish(m *model.Message) error {
	log.Printf("Persist not implemented, deviceID = %s", m.DeviceID)
	return nil
}

// AddNext ...
func (p *Persist) AddNext(pe Pipeline) {
	p.next = pe
}

// Next ...
func (p *Persist) Next() Pipeline {
	return p.next
}

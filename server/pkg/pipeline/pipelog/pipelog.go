package pipelog

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
)

// Log is a pipeline processor that logs incoming messages
type Log struct {
	next pipeline.Pipeline
	opts *opts.Opts
}

// New creates new instance of Log pipeline element
func New(opts *opts.Opts) *Log {
	return &Log{
		opts: opts,
	}
}

// Publish ...
func (p *Log) Publish(m *model.Message) error {
	log.Printf("Message: device='%s' messageID=%d packetSize=%d", m.DeviceID, m.ID, m.PacketSize)

	if p.next != nil {
		return p.next.Publish(m)
	}
	return nil
}

// AddNext ...
func (p *Log) AddNext(pe pipeline.Pipeline) {
	p.next = pe
}

// Next ...
func (p *Log) Next() pipeline.Pipeline {
	return p.next
}

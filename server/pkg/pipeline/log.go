package pipeline

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
)

// Log is a pipeline processor that logs incoming messages
type Log struct {
	next Pipeline
	opts *opts.Opts
}

// NewLog creates new instance of Log pipeline element
func NewLog(opts *opts.Opts) *Log {
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
func (p *Log) AddNext(pe Pipeline) {
	p.next = pe
}

// Next ...
func (p *Log) Next() Pipeline {
	return p.next
}

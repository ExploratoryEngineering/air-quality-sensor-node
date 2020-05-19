package persist

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
)

// Persist is a pipeline processor that persists incoming messages
type Persist struct {
	db   store.Store
	next pipeline.Pipeline
	opts *opts.Opts
}

// New creates new Persist pipeline element
func New(opts *opts.Opts, db store.Store) *Persist {
	return &Persist{
		opts: opts,
		db:   db,
	}
}

// Publish ...
func (p *Persist) Publish(m *model.Message) error {
	id, err := p.db.PutMessage(m)
	if err != nil {
		log.Printf("Error logging message: %v", err)
	} else {
		// Populate with storage ID
		m.ID = id
	}

	if p.next != nil {
		return p.next.Publish(m)
	}
	return nil
}

// AddNext ...
func (p *Persist) AddNext(pe pipeline.Pipeline) {
	p.next = pe
}

// Next ...
func (p *Persist) Next() pipeline.Pipeline {
	return p.next
}

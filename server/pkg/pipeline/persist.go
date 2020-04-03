package pipeline

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
)

// Persist is a pipeline processor that persists incoming messages
type Persist struct {
	db   store.Store
	next Pipeline
	opts *opts.Opts
}

// NewPersist creates new Persist pipeline element
func NewPersist(opts *opts.Opts, db store.Store) *Persist {
	return &Persist{
		opts: opts,
		db:   db,
	}
}

// Publish ...
func (p *Persist) Publish(m *model.Message) error {
	id, err := p.db.PutMessage(m)
	if err != nil {
		return err
	}

	// Populate with storage ID
	m.ID = id

	if p.next != nil {
		return p.next.Publish(m)
	}
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

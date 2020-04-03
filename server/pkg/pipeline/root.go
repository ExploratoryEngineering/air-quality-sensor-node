// The root pipeline element is responsible for ensuring that when a
// message comes in, the device is known to us.  If the device is
// unknown, it is populated into the database.
//
// TODO(borud): implement database updates

package pipeline

import (
	"errors"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
)

// ErrEmptyPipeline indicates that the Root pipeline has no next
// element.  Which makes it kind of useless.
var ErrEmptyPipeline = errors.New("Empty pipeline, has no next element")

// Root is the root handler for pipelines.
type Root struct {
	next Pipeline
	opts *opts.Opts
}

// NewRoot creates a new Root instance
func NewRoot(opts *opts.Opts) *Root {
	return &Root{
		opts: opts,
	}
}

// Publish ...
func (p *Root) Publish(m *model.Message) error {
	if p.next != nil {
		return p.next.Publish(m)
	}
	return nil
}

// AddNext ...
func (p *Root) AddNext(pe Pipeline) {
	p.next = pe
}

// Next ...
func (p *Root) Next() Pipeline {
	return p.next
}

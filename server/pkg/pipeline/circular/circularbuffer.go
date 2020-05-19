package circular

import (
	"container/ring"
	"sync"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
)

// Buffer is a ring-buffer holding the last N messages received.
type Buffer struct {
	ring *ring.Ring
	mu   sync.Mutex
	next pipeline.Pipeline
}

// New creates a new ring buffer pipeline element.
func New(size int) *Buffer {
	return &Buffer{
		ring: ring.New(size),
	}
}

// Publish adds a message to the ring buffer
func (c *Buffer) Publish(m *model.Message) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.ring.Value = m
	c.ring = c.ring.Next()

	if c.next != nil {
		return c.next.Publish(m)
	}
	return nil
}

// GetContents returns the contents of the circular buffer
func (c *Buffer) GetContents() []*model.Message {
	c.mu.Lock()
	defer c.mu.Unlock()

	var messages []*model.Message

	c.ring.Do(func(p interface{}) {
		if p != nil {
			messages = append(messages, p.(*model.Message))
		}
	})
	return messages
}

// AddNext adds a Pipeline element after this pipeline element
func (c *Buffer) AddNext(pe pipeline.Pipeline) {
	c.next = pe
}

// Next returns the next pipeline element
func (c *Buffer) Next() pipeline.Pipeline {
	return c.next
}

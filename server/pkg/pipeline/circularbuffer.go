package pipeline

import (
	"container/ring"
	"sync"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// CircularBuffer is a ring-buffer holding the last N messages
// received.
type CircularBuffer struct {
	ring *ring.Ring
	mu   sync.Mutex
	next Pipeline
}

// NewCircularBuffer creates a new ring buffer pipeline element.
func NewCircularBuffer(size int) *CircularBuffer {
	return &CircularBuffer{
		ring: ring.New(size),
	}
}

// Publish adds a message to the ring buffer
func (c *CircularBuffer) Publish(m *model.Message) error {
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
func (c *CircularBuffer) GetContents() []*model.Message {
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
func (c *CircularBuffer) AddNext(pe Pipeline) {
	c.next = pe
}

// Next returns the next pipeline element
func (c *CircularBuffer) Next() Pipeline {
	return c.next
}

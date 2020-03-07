package pipeline

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// Pipeline defines the interface of processing pipeline elements.
type Pipeline interface {
	Publish(m *model.Message) error
	AddNext(pe Pipeline)
	Next() Pipeline
}

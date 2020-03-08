package store

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// Store defines the persistence interface.
type Store interface {
	// Device
	PutDevice(d *model.Device) error
	GetDevice(id string) (*model.Device, error)
	UpdateDevice(d *model.Device) error
	DeleteDevice(id string) error

	// Message
	PutMessage(m *model.Message) error
	GetMessage(id int64) (*model.Message, error)
	ListMessages(offset int, limit int) ([]model.Message, error)

	// Calibration
	PutCal(c *model.Cal) error
	GetCal(id int64) (*model.Cal, error)
	DeleteCal(id int64) error
	ListCal(deviceID string) ([]model.Cal, error)
}

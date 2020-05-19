package store

import (
	"errors"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// ErrCalExists indicates that calibration entry already exists
var ErrCalExists = errors.New("Calibration entry already exists")

// Store defines the persistence interface.
type Store interface {
	// ############################################################
	//                     Calibration
	// ############################################################

	// PutCal adds a new calibration entry.  Note that the device
	// referred to must already exist in the database.
	PutCal(c *model.Cal) (int64, error)

	// GetCal gets a calibration entry by id
	GetCal(id int64) (*model.Cal, error)

	// DeleteCal deletes calibration entry.
	DeleteCal(id int64) error

	// ListCal lists calibration data ordered by DeviceID and ValidFrom in ascending order.
	ListCals(offset int, limit int) ([]model.Cal, error)

	// ListCalForDevice lists calibration data for device ordered by
	// validFrom date in descending order.
	ListCalsForDevice(deviceID string) ([]model.Cal, error)

	// ############################################################
	//                     Message
	// ############################################################

	// PutMessage adds a new message to database
	PutMessage(m *model.Message) (int64, error)

	// GetMessage gets a message by id
	GetMessage(id int64) (*model.Message, error)

	// ListMessages pages through messages.  Messages are sorted in descending order by ReceivedTime.
	ListMessages(offset int, limit int) ([]model.Message, error)

	// ListMessagesByDate lists messages by date [from:to>
	ListMessagesByDate(from int64, to int64) ([]model.Message, error)

	// ListDeviceMessagesByDate lists messages by device and date [from:to>
	ListDeviceMessagesByDate(deviceID string, from int64, to int64) ([]model.Message, error)

	// Close the database
	Close() error
}

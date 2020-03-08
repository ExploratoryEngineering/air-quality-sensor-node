package store

import (
	"fmt"
	"math/rand"
	"testing"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
	"github.com/stretchr/testify/assert"
)

var testDevice = model.Device{
	ID:           "device1",
	Name:         "testdevice",
	CollectionID: "collection1",
}

var testCal = model.Cal{}

func TestSqlitestore(t *testing.T) {

	// Device tests
	{
		var db Store

		db, err := sqlitestore.New(":memory:")
		assert.Nil(t, err, "Error instantiating new sqlitestore")
		assert.NotNil(t, db)
		deviceTests(t, db)
		db.Close()
	}

	// Cal tests
	{
		var db Store

		db, err := sqlitestore.New(":memory:")
		assert.Nil(t, err, "Error instantiating new sqlitestore")
		assert.NotNil(t, db)
		calTests(t, db)
		db.Close()
	}

	// Message tests
	{
		var db Store

		db, err := sqlitestore.New(":memory:")
		assert.Nil(t, err, "Error instantiating new sqlitestore")
		assert.NotNil(t, db)
		messageTests(t, db)
		db.Close()
	}

}

// deviceTests performs CRUD tests on Device
func deviceTests(t *testing.T, db Store) {

	// Put
	{
		err := db.PutDevice(&testDevice)
		assert.Nil(t, err)
	}

	// Get
	{
		d, err := db.GetDevice(testDevice.ID)
		assert.Nil(t, err)
		assert.NotNil(t, d)
		assert.Equal(t, testDevice, *d)
	}

	// Update
	{
		ud := testDevice
		ud.Name = "modified name"

		err := db.UpdateDevice(&ud)
		assert.Nil(t, err)

		d, err := db.GetDevice(testDevice.ID)
		assert.Nil(t, err)
		assert.NotNil(t, d)
		assert.Equal(t, testDevice.CollectionID, d.CollectionID)
		assert.NotEqual(t, testDevice.Name, d.Name)
	}

	// Delete
	{
		err := db.DeleteDevice(testDevice.ID)
		assert.Nil(t, err)

		d, err := db.GetDevice(testDevice.ID)
		assert.NotNil(t, err)
		assert.Nil(t, d)
	}

	// List
	{
		for i := 0; i < 20; i++ {
			err := db.PutDevice(&model.Device{ID: fmt.Sprintf("device-%d", i)})
			assert.Nil(t, err)
		}

		devices, err := db.ListDevices(0, 100)
		assert.Nil(t, err)
		assert.Equal(t, 20, len(devices))
	}
}

// calTests performs CRUD tests on Cal
func calTests(t *testing.T, db Store) {

	{
		// Put
		id, err := db.PutCal(&testCal)
		assert.Nil(t, err)
		assert.True(t, id > 0)

		testCal.ID = id

		// Get
		c, err := db.GetCal(id)
		assert.Nil(t, err)
		assert.NotNil(t, c)
		assert.Equal(t, testCal, *c)
	}

	// Update
	{
		uc := testCal
		uc.ValidTo = time.Now()

		err := db.UpdateCal(&uc)
		assert.Nil(t, err)

		c, err := db.GetCal(testCal.ID)
		assert.Nil(t, err)
		assert.NotNil(t, c)
		assert.Equal(t, testCal.DeviceID, c.DeviceID)
		assert.NotEqual(t, testCal.ValidTo, c.ValidTo)
	}

	// Delete
	{
		err := db.DeleteCal(testCal.ID)
		assert.Nil(t, err)

		c, err := db.GetCal(testCal.ID)
		assert.NotNil(t, err)
		assert.Nil(t, c)
	}

	// ListCals
	{
		for i := 0; i < 20; i++ {
			err := db.PutDevice(&model.Device{ID: fmt.Sprintf("cal-device-%d", i)})
			assert.Nil(t, err)

			id, err := db.PutCal(&model.Cal{
				DeviceID:  fmt.Sprintf("cal-device-%d", i),
				ValidFrom: time.Now().Add(-24 * time.Hour),
				ValidTo:   time.Now().Add(24 * time.Hour),
			})
			assert.Nil(t, err)
			assert.True(t, id > 0)
		}

		cals, err := db.ListCals(0, 100)
		assert.Nil(t, err)
		assert.Equal(t, 20, len(cals))

		// ListCalsForDevice
		devices, err := db.ListDevices(0, 100)
		assert.Nil(t, err)
		assert.Equal(t, 20, len(devices))

		// Make sure that there is one cal for each device
		for _, dev := range devices {
			devcals, err := db.ListCalsForDevice(dev.ID)
			assert.Nil(t, err)
			assert.Equal(t, 1, len(devcals))
		}
	}

}

// messageTests performs CRUD tests on Messages
func messageTests(t *testing.T, db Store) {

	numDevices := 3
	numMessagesPerDevice := (60 * 24)
	t0 := time.Now()

	var messageIDs []int64

	// Populate some devices and messages
	for i := 0; i < numDevices; i++ {
		deviceID := fmt.Sprintf("msg-device-%d", i)
		err := db.PutDevice(&model.Device{ID: deviceID})
		assert.Nil(t, err)

		for j := 0; j < numMessagesPerDevice; j++ {
			id, err := db.PutMessage(&model.Message{
				DeviceID:     deviceID,
				ReceivedTime: t0.Add(time.Duration(j) * time.Minute),
			})
			assert.Nil(t, err)
			assert.True(t, id > 0)

			messageIDs = append(messageIDs, id)
		}
	}

	totalMessages := numMessagesPerDevice * numDevices
	assert.Equal(t, totalMessages, len(messageIDs))

	// Fetch some random messages
	for i := 0; i < 20; i++ {
		id := rand.Int63n(int64(totalMessages))

		m, err := db.GetMessage(id)
		assert.Nil(t, err)
		assert.NotNil(t, m)
	}

	// ListMessages
	{
		msgs, err := db.ListMessages(0, 10)
		assert.Nil(t, err)
		assert.NotNil(t, msgs)
		assert.Equal(t, 10, len(msgs))
	}

	// ListMessagesByDate
	{
		duration := time.Minute * 10
		msgs, err := db.ListMessagesByDate(t0, t0.Add(duration))
		assert.Nil(t, err)
		assert.Equal(t, numDevices*10, len(msgs))
	}

	// ListDeviceMessagesByDate
	{
		for i := 0; i < numDevices; i++ {
			deviceID := fmt.Sprintf("msg-device-%d", i)
			duration := time.Minute * 10
			msgs, err := db.ListDeviceMessagesByDate(deviceID, t0, t0.Add(duration))
			assert.Nil(t, err)
			assert.Equal(t, 10, len(msgs))
		}
	}

}

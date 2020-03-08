package store

import (
	"testing"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
	"github.com/stretchr/testify/assert"
)

var testDevice = model.Device{
	ID:           "device1",
	Name:         "testdevice",
	CollectionID: "collection1",
}

func TestSqlitestore(t *testing.T) {
	var db Store

	db, err := sqlitestore.New(":memory:")
	assert.Nil(t, err, "Error instantiating new sqlitestore")
	assert.NotNil(t, db)

	// Put
	err = db.PutDevice(&testDevice)
	assert.Nil(t, err)

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

		err = db.UpdateDevice(&ud)
		assert.Nil(t, err)

		d, err := db.GetDevice(testDevice.ID)
		assert.Nil(t, err)
		assert.NotNil(t, d)
		assert.Equal(t, testDevice.ID, d.ID)
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
}

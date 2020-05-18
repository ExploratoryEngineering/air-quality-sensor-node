package pipelinetest

import (
	"testing"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/calculate"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/persist"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/pipelog"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
	"github.com/stretchr/testify/assert"
)

var testMessage = &model.Message{
	DeviceID:    "my-device-id",
	PacketSize:  123,
	Sensor1Work: 519656,
	Sensor1Aux:  522293,

	Sensor2Work: 697336,
	Sensor2Aux:  682847,

	Sensor3Work: 443429,
	Sensor3Aux:  431900,

	AFE3TempRaw: 540375,
}

var testCal = &model.Cal{
	DeviceID: "my-device-id",
	// value from deviceID '17dh0cf43jg6n4'. Measured by Thomas Langås
	// in the lab under relatively stable conditions.  Can use 0.32 as
	// default VT20Offset if we lack measured offset value.
	Vt20Offset: 0.3195,

	Sensor1WEe:           312,
	Sensor1WE0:           -5,
	Sensor1AEe:           316,
	Sensor1AE0:           -5,
	Sensor1PCBGain:       -0.73,
	Sensor1WESensitivity: 0.203,

	Sensor2WEe:           411,
	Sensor2WE0:           -4,
	Sensor2AEe:           411,
	Sensor2AE0:           -3,
	Sensor2PCBGain:       -0.73,
	Sensor2WESensitivity: 0.363,

	Sensor3WEe:           271,
	Sensor3WE0:           19,
	Sensor3AEe:           256,
	Sensor3AE0:           23,
	Sensor3PCBGain:       0.8,
	Sensor3WESensitivity: 0.408,
}

func TestRoot(t *testing.T) {
	db, err := sqlitestore.New(":memory:")
	assert.Nil(t, err)
	assert.NotNil(t, db)

	opts := &opts.Opts{
		HordeCollection: "abc",
		DBFilename:      ":memory:",
	}

	root := pipeline.New(opts, db)
	assert.NotNil(t, root)

	root.Publish(testMessage)
}

func TestPipeline(t *testing.T) {
	db, err := sqlitestore.New(":memory:")
	assert.Nil(t, err)
	assert.NotNil(t, db)

	_, err = db.PutCal(testCal)
	assert.Nil(t, err)

	opts := &opts.Opts{
		HordeCollection: "abc",
		DBFilename:      ":memory:",
	}

	root := pipeline.New(opts, db)
	calculate := calculate.New(opts, db)
	persist := persist.New(opts, db)
	logger := pipelog.New(opts)
	circular := circular.New(10)

	root.AddNext(calculate)
	calculate.AddNext(persist)
	persist.AddNext(logger)
	logger.AddNext(circular)

	// Make defensive copy
	msg := *testMessage

	// Ensure the message is modified (new values are calculated)
	assert.Equal(t, testMessage, &msg)
	root.Publish(&msg)
	assert.NotEqual(t, testMessage, &msg)

	// Make sure that message has been persisted
	m, err := db.GetMessage(msg.ID)
	assert.Nil(t, err)
	assert.Equal(t, msg.ID, m.ID)

	// Make sure there's at least one message in the circular buffer
	msgs := circular.GetContents()
	assert.NotNil(t, msgs)
	assert.Equal(t, 1, len(msgs))
}

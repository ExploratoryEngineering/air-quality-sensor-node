package pipeline

import (
	"testing"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/stretchr/testify/assert"
)

var cals = []model.Cal{
	model.Cal{DeviceID: "foo", ID: 3, ValidFrom: time.Date(2002, 1, 30, 0, 0, 0, 0, time.UTC)},
	model.Cal{DeviceID: "foo", ID: 2, ValidFrom: time.Date(2001, 1, 30, 0, 0, 0, 0, time.UTC)},
	model.Cal{DeviceID: "foo", ID: 1, ValidFrom: time.Date(2000, 1, 30, 0, 0, 0, 0, time.UTC)},
}

func TestFindCacheEntry(t *testing.T) {
	c := &Calculate{}
	c.populateCache(cals)

	assert.Equal(t, int64(1), c.findCacheEntry("foo", ms(time.Date(2000, 2, 30, 0, 0, 0, 0, time.UTC))).ID)
	assert.Equal(t, int64(2), c.findCacheEntry("foo", ms(time.Date(2001, 2, 30, 0, 0, 0, 0, time.UTC))).ID)
	assert.Equal(t, int64(3), c.findCacheEntry("foo", ms(time.Date(2003, 1, 1, 0, 0, 0, 0, time.UTC))).ID)

	// We want the oldest entry if the data precedes calibration data.
	// This is defined behavior but not necessarily smart behavior.
	assert.Equal(t, int64(1), c.findCacheEntry("foo", ms(time.Date(1999, 2, 30, 0, 0, 0, 0, time.UTC))).ID)

	// Check for exact coincidence
	assert.Equal(t, int64(1), c.findCacheEntry("foo", ms(time.Date(2000, 1, 30, 0, 0, 0, 0, time.UTC))).ID)
	assert.Equal(t, int64(2), c.findCacheEntry("foo", ms(time.Date(2001, 1, 30, 0, 0, 0, 0, time.UTC))).ID)
	assert.Equal(t, int64(3), c.findCacheEntry("foo", ms(time.Date(2003, 1, 30, 0, 0, 0, 0, time.UTC))).ID)
}

// Convert time.Time to milliseconds since epoch
func ms(t time.Time) int64 {
	return t.UnixNano() / int64(time.Millisecond)
}

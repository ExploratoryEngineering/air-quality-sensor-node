package pipeline

import (
	"log"
	"sort"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
)

// Calculate holds the configuration the calculation processor.
type Calculate struct {
	next             Pipeline
	opts             *opts.Opts
	db               store.Store
	calibrationCache map[string][]model.Cal
	cacheRefreshChan chan bool
	quitChan         chan bool
}

const (
	maxint = ^0 >> 1 // no idea why maxint doesn't already exist
)

// NewCalculate creates a new instance of Calculate pipeline element
func NewCalculate(opts *opts.Opts, db store.Store) *Calculate {
	c := &Calculate{
		opts:             opts,
		db:               db,
		cacheRefreshChan: make(chan bool),
	}

	// Do initial population of cache
	cals, err := db.ListCals(0, maxint)
	if err != nil {
		log.Fatalf("Unable to pre-populate calibration cache: %v", err)
	}

	c.populateCache(cals)

	// Start cache refresh goroutine
	go c.cacheRefresh()

	return c
}

func (p *Calculate) populateCache(cals []model.Cal) {
	m := make(map[string][]model.Cal)

	for _, cal := range cals {
		m[cal.DeviceID] = append(m[cal.DeviceID], cal)
	}

	// Since the findCacheEntry algorithm absolutely depends on the
	// correct date order and we shouldn't depend on the store layer
	// doing things correctly (even though ordering is specified)
	// people can screw up and change that.  So better safe than
	// sorry.
	for _, v := range m {
		sort.Slice(v, func(i, j int) bool {
			return v[i].ValidFrom.After(v[j].ValidFrom)
		})
	}

	p.calibrationCache = m
}

func (p *Calculate) cacheRefresh() {
	for {
		log.Printf("Cache refresh...")

		select {
		case <-p.quitChan:
			log.Printf("Terminating cache refresher")

		case <-p.cacheRefreshChan:
			log.Printf("Refresh cache")
		}

	}
}

// findCacheEntry assumes that the calibration entries are sorted in
// descending order by date in the cache.
func (p *Calculate) findCacheEntry(deviceID string, t int64) *model.Cal {
	deviceCalEntries := p.calibrationCache[deviceID]

	date := time.Unix(0, t*int64(time.Millisecond))

	var cal model.Cal
	for i := 0; i < len(deviceCalEntries); i++ {
		cal = deviceCalEntries[i]
		if !date.Before(cal.ValidFrom) {
			break
		}
	}

	return &cal
}

// Publish ...
func (p *Calculate) Publish(m *model.Message) error {
	cal := p.findCacheEntry(m.DeviceID, m.ReceivedTime)
	model.CalculateSensorValues(m, cal)

	if p.next != nil {
		return p.next.Publish(m)
	}
	return nil
}

// AddNext ...
func (p *Calculate) AddNext(pe Pipeline) {
	p.next = pe
}

// Next ...
func (p *Calculate) Next() Pipeline {
	return p.next
}

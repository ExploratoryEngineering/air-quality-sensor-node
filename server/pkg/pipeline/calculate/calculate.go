package calculate

import (
	"log"
	"sort"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
)

// Calculate holds the configuration the calculation processor.
type Calculate struct {
	next             pipeline.Pipeline
	opts             *opts.Opts
	db               store.Store
	calibrationCache map[string][]model.Cal
	cacheRefreshChan chan bool
	lastCacheUpdate  time.Time
}

const (
	maxint              = ^0 >> 1 // no idea why maxint doesn't already exist
	minCacheUpdateDelay = (5 * time.Second)
)

// New creates a new instance of Calculate pipeline element
func New(opts *opts.Opts, db store.Store) *Calculate {
	c := &Calculate{
		opts:             opts,
		db:               db,
		cacheRefreshChan: make(chan bool),
	}

	err := c.loadCache()
	if err != nil {
		log.Fatalf("Unable to pre-populate calibration cache: %v", err)
	}
	return c
}

func (p *Calculate) loadCache() error {
	cals, err := p.db.ListCals(0, maxint)
	if err != nil {
		return err
	}

	p.populateCache(cals)

	return nil
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
	p.lastCacheUpdate = time.Now()
}

// findCacheEntry assumes that the calibration entries are sorted in
// descending order by date in the cache.
func (p *Calculate) findCacheEntry(deviceID string, t int64) *model.Cal {

	// Somewhat hokey caching logic.  Replace this nonsense with a
	// proper caching layer that uses the Store interface.
	refreshedCache := false
	var deviceCalEntries []model.Cal
	for {
		deviceCalEntries = p.calibrationCache[deviceID]
		if deviceCalEntries != nil {
			// We found cache entry so bail out
			break
		}

		// We did not find a cached entry.  If we have already
		// refreshed, we bail and accept the consequences.
		if refreshedCache {
			log.Printf("Missing calibration data for '%s' (will only report every %.2f seconds)", deviceID, minCacheUpdateDelay.Seconds())
			break
		}

		// Check when we last updated cache.  If it is less than
		// minCacheUpdateDelay we skip the update
		if time.Now().Before(p.lastCacheUpdate.Add(minCacheUpdateDelay)) {
			break
		}

		// We load refresh the cache and go around once more
		err := p.loadCache()
		if err != nil {
			log.Printf("Error updating cache, continuing with possibly stale data: %v", err)
		}
		refreshedCache = true
		log.Print("Refreshed calibration data cache")
	}

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
func (p *Calculate) AddNext(pe pipeline.Pipeline) {
	p.next = pe
}

// Next ...
func (p *Calculate) Next() pipeline.Pipeline {
	return p.next
}

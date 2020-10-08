package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
	sqlite3 "github.com/mattn/go-sqlite3"
)

// DefaultFilenamePattern is the default pattern to be used for
// reading the calibration data files.
const DefaultFilenamePattern = "*.json"

// checkForNewCalibrationData fetches the calibration data from the
// distribution point in S3 and attempts to insert it into the
// database.  If it already exists the database will just reject it
// with a constraint error.
func loadCalibrationData(db store.Store, dir string) error {
	cals := make(map[uint64]*model.Cal)
	pattern := DefaultFilenamePattern

	f := func(path string, info os.FileInfo, err error) error {
		if info.IsDir() {
			return nil
		}

		match, err := filepath.Match(pattern, info.Name())
		if err != nil {
			log.Printf("Error matching '%s': %v", pattern, err)
			return err
		}

		if !match {
			log.Printf("SKIP '%s', no match for %s", info.Name(), pattern)
			return nil
		}

		data, err := ioutil.ReadFile(path)
		if err != nil {
			log.Printf("SKIP '%s', error reading : %v", path, err)
			return err
		}

		var cal model.Cal
		err = json.Unmarshal(data, &cal)
		if err != nil {
			log.Printf("SKIP '%s' unable to parse : %v", path, err)
			return err
		}

		// Check if SysID is present.  If it is not the calibration
		// file cannot be used and will be skipped.
		if cal.SysID == 0 {
			log.Printf("SKIP '%s' Not a valid calibration file: SysID missing", path)
			return nil
		}

		cals[cal.SysID] = &cal
		return nil
	}

	dirInfo, err := os.Stat(dir)
	if err != nil {
		return err
	}

	if !dirInfo.IsDir() {
		return fmt.Errorf("Calibration data directory '%s' does not exist", dir)
	}

	err = filepath.Walk(dir, f)
	if err != nil {
		return err
	}

	newCalibrationSets := 0
	calibrationSetCount := 0

	for _, v := range cals {
		calibrationSetCount++
		_, err := db.PutCal(v)
		if err != nil {
			e, ok := err.(sqlite3.Error)
			if !(ok && e.Code == sqlite3.ErrConstraint) {
				log.Printf("Error loading calibration data: %v", err)
			}
		} else {
			log.Printf("Adding calibration data for %s", v.DeviceID)
			newCalibrationSets++
		}
	}

	log.Printf("Read %d calibration files from %s and found %d new sets.", calibrationSetCount, dir, newCalibrationSets)

	return nil
}

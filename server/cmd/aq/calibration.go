package main

import (
	"log"
	"strings"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/caldata"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
)

// checkForNewCalibrationData fetches the calibration data from the
// distribution point in S3 and attempts to insert it into the
// database.  If it already exists the database will just reject it
// with a constraint error.
func checkForNewCalibrationData(db store.Store) error {
	c, err := caldata.NewCaldata(options.CalDataS3URL)
	if err != nil {
		return err
	}

	// Get all the calibration data available from the S3 distribution point
	cals, err := c.DownloadFromS3()
	if err != nil {
		return err
	}

	// Loop through it and see if we have calibration data for the device
	for _, cal := range cals {
		// Just try to insert it and see if it succeeds, if we get a
		// constraint error we know the calibration data is there
		// already
		_, err := db.PutCal(&cal)
		if err != nil {
			if !strings.Contains(err.Error(), "UNIQUE constraint failed") {
				log.Printf("Error inserting calibration data: %+v", err)
			}
		}
	}

	log.Printf("Downloaded calibration data for %d devices", len(cals))

	return nil
}

// periodicCheckForNewCalibrationData sleeps for
// checkForNewCalibrationDataPeriod and then downloads calibration
// data and attempts to insert it.
func periodicCheckForNewCalibrationData(db store.Store) {
	for {
		time.Sleep(checkForNewCalibrationDataPeriod)
		log.Printf("Checking for new calibration data")
		err := checkForNewCalibrationData(db)
		if err != nil {
			log.Printf("Failed to check for new calibration data: %v", err)
		}
	}
}

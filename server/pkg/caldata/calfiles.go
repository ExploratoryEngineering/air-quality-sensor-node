package caldata

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// DefaultFilenamePattern is the default pattern to be used for
// reading the calibration data files.
const DefaultFilenamePattern = "*.json"

// Read all files matching the filename pattern in dir or directories
// below and return an array of Cal calibration instances.  This will
// assume all files that match the pattern are possible Cal instances
func readCalsFromDir(dir string, pattern string) (map[uint64]*model.Cal, error) {
	cals := make(map[uint64]*model.Cal)

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

	err := filepath.Walk(dir, f)
	if err != nil {
		return cals, err
	}

	return cals, nil
}

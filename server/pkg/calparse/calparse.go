// calparse is a package for parsing the CSV files from Alphasense.
// The CSV files do not make a whole lot of sense since they seem to
// be just a raw dump of some Excel spreadsheet, so assume that the
// format will be very fragile and could break in the near future.
package calparse

import (
	"bufio"
	"encoding/csv"
	"fmt"
	"log"
	"os"
	"strconv"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

type pos struct {
	row int
	col int
}

const (
	// Vt20Offset - value from deviceID '17dh0cf43jg6n4'. Measured by
	// Thomas Langås in the lab under relatively stable conditions.
	// Can use 0.32 as default VT20Offset if we lack measured offset
	// value.
	Vt20Offset = 0.3195
)

var (
	p = map[string]pos{
		"AFECalDate":  pos{6, 2},
		"AFESerial":   pos{7, 2},
		"AFEType":     pos{10, 2},
		"CircuitType": pos{11, 2},

		// TODO(borud): perhaps make use of these?
		// "Sensor1Type": pos{15, 3},
		// "Sensor2Type": pos{15, 4},
		// "Sensor3Type": pos{15, 5},

		"Sensor1Serial": pos{16, 3},
		"Sensor2Serial": pos{16, 4},
		"Sensor3Serial": pos{16, 5},

		"Sensor1WEe": pos{18, 3},
		"Sensor2WEe": pos{18, 4},
		"Sensor3WEe": pos{18, 5},

		"Sensor1WE0": pos{19, 3},
		"Sensor2WE0": pos{19, 4},
		"Sensor3WE0": pos{19, 5},

		"Sensor1AEe": pos{21, 3},
		"Sensor2AEe": pos{21, 4},
		"Sensor3AEe": pos{21, 5},

		"Sensor1AE0": pos{22, 3},
		"Sensor2AE0": pos{22, 4},
		"Sensor3AE0": pos{22, 5},

		"Sensor1PCBGain": pos{27, 3},
		"Sensor2PCBGain": pos{27, 4},
		"Sensor3PCBGain": pos{27, 5},

		"Sensor1WESensitivity": pos{28, 3},
		"Sensor2WESensitivity": pos{28, 4},
		"Sensor3WESensitivity": pos{28, 5},
	}
)

func lookup(data [][]string, name string) string {
	ent := p[name]
	return data[ent.row][ent.col]
}

func lookupInt32(data [][]string, name string) int32 {
	ent := p[name]
	s := data[ent.row][ent.col]

	i, err := strconv.ParseInt(s, 10, 32)
	if err != nil {
		log.Printf("Unable to parse int32 '%s' = '%s': %v", name, s, err)
		return int32(0)
	}

	return int32(i)
}

func lookupFloat64(data [][]string, name string) float64 {
	ent := p[name]
	s := data[ent.row][ent.col]

	f, err := strconv.ParseFloat(s, 32)
	if err != nil {
		log.Printf("Unable to parse float64 '%s' = '%s': %v", name, s, err)
		return 0.0
	}

	return f
}

// ReadCalDataFromCSV reads the calibration data from CSV files we get
// from Alphasense.  How stable this CSV export is is anyone's guess,
// but it might be useful to come up with a better format for
// storing/sharing/transferring the calibration data.
//
func ReadCalDataFromCSV(filename string) (*model.Cal, error) {
	csvFile, err := os.Open(filename)
	if err != nil {
		return nil, fmt.Errorf("Unable to open %s: %v", filename, err)
	}
	defer csvFile.Close()

	r := csv.NewReader(bufio.NewReader(csvFile))
	dd, err := r.ReadAll()
	if err != nil {
		return nil, fmt.Errorf("Unable to read CSV file: %v", err)
	}

	caldate, err := time.Parse("02-01-2006", lookup(dd, "AFECalDate"))

	return &model.Cal{
		CircuitType:   lookup(dd, "CircuitType"),
		AFESerial:     lookup(dd, "AFESerial"),
		AFEType:       lookup(dd, "AFEType"),
		Sensor1Serial: lookup(dd, "Sensor1Serial"),
		Sensor2Serial: lookup(dd, "Sensor2Serial"),
		Sensor3Serial: lookup(dd, "Sensor3Serial"),
		AFECalDate:    caldate,
		Vt20Offset:    Vt20Offset,

		Sensor1WEe:           lookupInt32(dd, "Sensor1WEe"),
		Sensor1WE0:           lookupInt32(dd, "Sensor1WE0"),
		Sensor1AEe:           lookupInt32(dd, "Sensor1AEe"),
		Sensor1AE0:           lookupInt32(dd, "Sensor1AE0"),
		Sensor1PCBGain:       lookupFloat64(dd, "Sensor1PCBGain"),
		Sensor1WESensitivity: lookupFloat64(dd, "Sensor1WESensitivity"),

		Sensor2WEe:           lookupInt32(dd, "Sensor2WEe"),
		Sensor2WE0:           lookupInt32(dd, "Sensor2WE0"),
		Sensor2AEe:           lookupInt32(dd, "Sensor2AEe"),
		Sensor2AE0:           lookupInt32(dd, "Sensor2AE0"),
		Sensor2PCBGain:       lookupFloat64(dd, "Sensor2PCBGain"),
		Sensor2WESensitivity: lookupFloat64(dd, "Sensor2WESensitivity"),

		Sensor3WEe:           lookupInt32(dd, "Sensor3WEe"),
		Sensor3WE0:           lookupInt32(dd, "Sensor3WE0"),
		Sensor3AEe:           lookupInt32(dd, "Sensor3AEe"),
		Sensor3AE0:           lookupInt32(dd, "Sensor3WE0"),
		Sensor3PCBGain:       lookupFloat64(dd, "Sensor3PCBGain"),
		Sensor3WESensitivity: lookupFloat64(dd, "Sensor3WESensitivity"),
	}, nil
}

// The show command shows an entire calibration entry.
package main

import (
	"fmt"
	"log"
	"strconv"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// ShowCommand defines the command line parameters for show command
type ShowCommand struct {
	ToJSON bool `short:"j" long:"json" description:"Format calibration entry as JSON"`
}

var showCommand ShowCommand

func init() {
	parser.AddCommand("show",
		"Show calibration data for device",
		"Show calibration data for device, optionally formatting it as JSON",
		&showCommand)
}

// Execute runs the list command
func (a *ShowCommand) Execute(args []string) error {
	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		return err
	}
	defer db.Close()

	for _, idStr := range args {
		id, err := strconv.ParseInt(idStr, 10, 64)
		if err != nil {
			continue
		}

		cals, err := db.ListCalsForDevice(id)
		if err != nil {
			log.Fatalf("Unable to find calibration data for '%d': %v", id, err)
		}

		for _, cal := range cals {
			fmt.Print("--------------------------------------------------\n")
			fmt.Printf("  ID                   : %d\n", cal.ID)
			fmt.Printf("  DeviceID             : %s\n", cal.DeviceID)
			fmt.Printf("  CollectionID         : %s\n", cal.CollectionID)
			fmt.Printf("  ValidFrom            : %s\n", cal.ValidFrom)
			fmt.Printf("  CircuitType          : %s\n", cal.CircuitType)
			fmt.Printf("  AFESerial            : %s\n", cal.AFESerial)
			fmt.Printf("  AFEType              : %s\n", cal.AFEType)
			fmt.Printf("  Sensor1Serial        : %s\n", cal.Sensor1Serial)
			fmt.Printf("  Sensor2Serial        : %s\n", cal.Sensor2Serial)
			fmt.Printf("  Sensor3Serial        : %s\n", cal.Sensor3Serial)
			fmt.Printf("  AFECalDate           : %s\n", cal.AFECalDate)
			fmt.Printf("  Vt20Offset           : %f\n", cal.Vt20Offset)

			fmt.Printf("  Sensor1WEe           : %d\n", cal.Sensor1WEe)
			fmt.Printf("  Sensor1WE0           : %d\n", cal.Sensor1WE0)
			fmt.Printf("  Sensor1AEe           : %d\n", cal.Sensor1AEe)
			fmt.Printf("  Sensor1AE0           : %d\n", cal.Sensor1WE0)
			fmt.Printf("  Sensor1PCBGain       : %f\n", cal.Sensor1PCBGain)
			fmt.Printf("  Sensor1WESensitivity : %f\n", cal.Sensor1WESensitivity)

			fmt.Printf("  Sensor2WEe           : %d\n", cal.Sensor2WEe)
			fmt.Printf("  Sensor2WE0           : %d\n", cal.Sensor2WE0)
			fmt.Printf("  Sensor2AEe           : %d\n", cal.Sensor2AEe)
			fmt.Printf("  Sensor2AE0           : %d\n", cal.Sensor2AE0)
			fmt.Printf("  Sensor2PCBGain       : %f\n", cal.Sensor2PCBGain)
			fmt.Printf("  Sensor2WESensitivity : %f\n", cal.Sensor2WESensitivity)

			fmt.Printf("  Sensor3WEe           : %d\n", cal.Sensor3WEe)
			fmt.Printf("  Sensor3WE0           : %d\n", cal.Sensor3WE0)
			fmt.Printf("  Sensor3AEe           : %d\n", cal.Sensor3AEe)
			fmt.Printf("  Sensor3AE0           : %d\n", cal.Sensor3AE0)
			fmt.Printf("  Sensor3PCBGain       : %f\n", cal.Sensor3PCBGain)
			fmt.Printf("  Sensor3WESensitivity : %f\n", cal.Sensor3WESensitivity)
			fmt.Print("--------------------------------------------------\n")
		}
	}
	return nil
}

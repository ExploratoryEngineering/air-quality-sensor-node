// The parse command is mostly there to check that we can parse the
// CSV file(s) and visually check that a calibration file looks okay.
//
package main

import (
	"fmt"
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// ListCommand defines the command line parameters for list command
type ListCommand struct {
	Offset int `short:"o" long:"offset" description:"offset from lowest id" default:"0"`
	Limit  int `short:"l" long:"limit" description:"Number of entries to show" default:"100"`
}

var listCommand ListCommand

func init() {
	parser.AddCommand("ls",
		"List calibration data",
		"List calibration data",
		&listCommand)
}

// Execute runs the list command
func (a *ListCommand) Execute(args []string) error {
	db, err := sqlitestore.New(options.DBFilename)
	defer db.Close()

	cals, err := db.ListCals(a.Offset, a.Limit)
	if err != nil {
		log.Fatalf("Unable to list calibration data: %v", err)
	}

	for _, cal := range cals {
		fmt.Printf("%d, %s, %s, %s, %s\n", cal.ID, cal.DeviceID, cal.ValidFrom.Format(layout), cal.ValidTo.Format(layout), cal.AFESerial)
	}
	return nil
}

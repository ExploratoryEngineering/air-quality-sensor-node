// The parse command is mostly there to check that we can parse the
// CSV file(s) and visually check that a calibration file looks okay.
//
package main

import (
	"log"
	"strconv"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// RemoveCommand defines the command line parameters for list command
type RemoveCommand struct{}

var removeCommand RemoveCommand

func init() {
	parser.AddCommand("rm",
		"Remove calibration data",
		"Remove calibration data",
		&removeCommand)
}

// Execute runs the list command
func (a *RemoveCommand) Execute(args []string) error {
	if len(args) < 1 {
		log.Fatalf("List IDs of calibration entries to be removed separated by spaces")
	}

	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open database '%s': %v", options.DBFilename, err)
	}
	defer db.Close()

	for _, idStr := range args {
		id, err := strconv.ParseInt(idStr, 10, 64)
		if err != nil {
			log.Printf("Invalid id '%s': %v", idStr, err)
			continue
		}

		err = db.DeleteCal(id)
		if err != nil {
			log.Fatalf("Unable to remove calibration entry: %v", err)
		}
	}
	return nil
}

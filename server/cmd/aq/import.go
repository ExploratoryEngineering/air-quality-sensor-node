// The import command imports calibration data from CSV file into the database.
package main

import (
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/calparse"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
	"github.com/araddon/dateparse"
)

// ImportCommand defines the command line parameters for import command
type ImportCommand struct {
	CollectionID string `short:"c" long:"collection-id" description:"ID of collection" default:"17dh0cf43jg007" value-name:"<id>"`
	DeviceID     string `short:"i" long:"device-id" description:"ID of device" value-name:"<id>"`
	ValidFrom    string `short:"f" long:"valid-from" description:"Start of validity period" value-name:"<date>"`
}

const (
	layout = "2006-01-02T15:04:05.000Z"
)

var importCommand ImportCommand

func init() {
	parser.AddCommand("import",
		"Import a calibration data",
		"The import command imports a the calibration data to the database",
		&importCommand)
}

// Execute runs the import command
func (a *ImportCommand) Execute(args []string) error {
	if len(args) < 1 {
		log.Fatalf("Please provide name of CSV file")
	}
	fileName := args[0]

	db, err := sqlitestore.New(options.DBFilename)
	defer db.Close()

	cal, err := calparse.ReadCalDataFromCSV(fileName)
	if err != nil {
		log.Fatal(err)
	}

	// If no valid from date is provided we will assume the calibration date of the device
	cal.ValidFrom = cal.AFECalDate
	if a.ValidFrom != "" {
		// Go has a comically stupid date parsing API so we
		// frivolously use a third party library for parsing this date
		cal.ValidFrom, err = dateparse.ParseStrict(a.ValidFrom)
		if err != nil {
			log.Fatalf("Error parsing ValidFrom date: %v", err)
		}
	}

	if cal.DeviceID == "" {
		cal.DeviceID = a.DeviceID
	}

	if cal.CollectionID == "" {
		cal.CollectionID = a.CollectionID
	}

	id, err := db.PutCal(cal)
	if err != nil {
		log.Fatalf("Unable to import calibration entry %s into database: %v", fileName, err)
	}

	log.Printf("Imported %s, entry id is '%d'", fileName, id)

	return nil
}

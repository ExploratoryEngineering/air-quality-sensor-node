// The import command imports calibration data from CSV file into the database.
package main

import (
	"encoding/json"
	"io/ioutil"
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// ImportCommand defines the command line parameters for import command
type ImportCommand struct {
	CalFetch bool   `short:"f" long:"fetch-cal" description:"Fetch calibration data from network"`
	CalURL   string `short:"u" long:"cal-url" description:"Distribution URL for calibration data" default:""`
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
		log.Fatalf("Please provide name of JSON file(s)")
	}

	a.importFiles(args)

	return nil
}

func (a *ImportCommand) importFiles(files []string) {
	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database: %v", err)
	}
	defer db.Close()

	for _, fileName := range files {
		data, err := ioutil.ReadFile(fileName)
		if err != nil {
			log.Printf("Cannot read %s, skipping: %v", fileName, err)
			continue
		}

		var cal model.Cal
		err = json.Unmarshal(data, &cal)
		if err != nil {
			log.Printf("Cannot unmarshal %s, skipping: %v", fileName, err)
			continue
		}

		// Do some validation
		if cal.DeviceID == "" {
			log.Printf("DeviceID is not set in %s, skipping", fileName)
			continue
		}

		// Override CollectionID if parameter is non-empty
		if options.HordeCollection != "" {
			cal.CollectionID = options.HordeCollection
		}

		id, err := db.PutCal(&cal)
		if err != nil {
			log.Fatalf("Unable to import calibration entry %s into database: %v", fileName, err)
		}

		log.Printf("Imported %s, CollectionID='%s', deviceID='%s', ID='%d'", fileName, cal.CollectionID, cal.DeviceID, id)
	}
}

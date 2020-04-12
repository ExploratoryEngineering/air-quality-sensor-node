// Convert Alphasense AFE calibration CSV files to JSON
//
package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"path"
	"path/filepath"
	"strings"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/calparse"
)

// ConvertCommand defines the command line parameters for convert command
type ConvertCommand struct {
	PrintJSON    bool   `short:"p" long:"print-json" description:"Output JSON to stdout"`
	OutpudDir    string `short:"o" long:"output-dir" description:"Output directory"`
	CollectionID string `short:"c" long:"collection-id" description:"ID of collection" default:"17dh0cf43jg007" value-name:"<id>"`
}

func init() {
	parser.AddCommand("convert",
		"Convert calibration data",
		"Convert calibration data from CSV to JSON format",
		&ConvertCommand{})
}

// Execute runs the convert command
func (a *ConvertCommand) Execute(args []string) error {
	if len(args) < 1 {
		log.Fatalf("Please provide filename(s) of CSV files")
	}

	for _, fileName := range args {
		ext := path.Ext(fileName)
		if !strings.EqualFold(".csv", ext) {
			log.Printf("%s is not a CSV file (has no '.csv' suffix), skipping", fileName)
			continue
		}

		cal, err := calparse.ReadCalDataFromCSV(fileName)
		if err != nil {
			log.Fatal(err)
		}

		cal.CollectionID = a.CollectionID

		// This is an okay default
		cal.ValidFrom = cal.AFECalDate

		json, err := json.MarshalIndent(cal, "  ", "")
		if err != nil {
			panic("Failed to marshal json")
		}

		if a.PrintJSON {
			fmt.Printf("%s", json)
			continue
		}

		outputFilename := strings.TrimSuffix(fileName, ext) + ".json"
		if a.OutpudDir != "" {
			outputFilename = filepath.Join(a.OutpudDir, filepath.Base(outputFilename))
		}

		log.Printf("Writing %s", outputFilename)
		err = ioutil.WriteFile(outputFilename, json, 0644)
		if err != nil {
			log.Fatalf("Error writing output file")
		}
	}
	return nil
}

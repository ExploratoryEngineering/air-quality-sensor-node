// The parse command is mostly there to check that we can parse the
// CSV file(s) and visually check that a calibration file looks okay.
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

// ParseCommand defines the command line parameters for parse command
type ParseCommand struct {
	WriteJSON bool   `short:"w" long:"write-json" description:"Output JSON file"`
	OutpudDir string `short:"o" long:"output-dir" description:"Output directory"`
}

var parseCommand ParseCommand

func init() {
	parser.AddCommand("parse",
		"Parse calibration data",
		"Parse calibration data and show the parsed output.  Mostly for visual inspection.",
		&parseCommand)
}

// Execute runs the parse command
func (a *ParseCommand) Execute(args []string) error {
	if len(args) < 1 {
		log.Fatalf("Please provide filename(s)")
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
		json, err := json.MarshalIndent(cal, "  ", "")
		if err != nil {
			panic("Failed to marshal json")
		}

		// If we are not writing to file we want the output on stdout
		if !a.WriteJSON {
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

// Download the calibration data to a directory
package main

import (
	"encoding/json"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/caldata"
)

// DownloadCommand defines the command line parameters for download command.
type DownloadCommand struct {
	OutputDir string `short:"o" long:"output-dir" description:"Output directory for calibration data" default:"" value-name:"<directory>"`
}

func init() {
	parser.AddCommand(
		"download",
		"Download the calibration data",
		"The Download command downloads the calibration data and writes them to JSON files",
		&DownloadCommand{},
	)
}

// Execute the download command
func (a *DownloadCommand) Execute(args []string) error {
	c, err := caldata.NewCaldata(options.CalDataS3URL)
	if err != nil {
		return err
	}

	// Figure out download directory and ensure it exists
	outputDir := a.OutputDir
	if outputDir == "" {
		outputDir = time.Now().Format("2006-01-02T150405-cal")
	}
	if _, err := os.Stat(outputDir); os.IsNotExist(err) {
		os.MkdirAll(outputDir, os.ModePerm)
	} else {
		log.Fatalf("Download directory %s already exists", outputDir)
	}

	log.Printf("Downloading data into directory '%s'", outputDir)

	// Get all the calibration data available from the S3 distribution point
	cals, err := c.DownloadFromS3()
	if err != nil {
		return err
	}

	for _, cal := range cals {
		json, err := json.MarshalIndent(&cal, "", "  ")
		if err != nil {
			log.Fatalf("Unable to marshal JSON")
		}

		filename := filepath.Join(outputDir, cal.AFESerial+".json")
		log.Printf("    - %s", filename)

		err = ioutil.WriteFile(filename, json, 0644)
		if err != nil {
			log.Printf("      FAILED: %v (continuing)", err)
		}
	}

	return nil
}

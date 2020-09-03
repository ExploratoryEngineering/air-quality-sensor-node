// Package opts contains the options that are common to app commands
package opts

// Opts contains command line options
type Opts struct {
	HordeCollection string `short:"c" long:"collection" description:"Horde collection id to listen to" default:"17dh0cf43jg007" value-name:"<collectionID>"`

	// Database options
	DBFilename string `short:"d" long:"db" description:"Data storage file" default:"aq.db" value-name:"<file>"`

	// Directory for calibration data
	CalibrationDataDir string `short:"m" long:"cal-data-dir" description:"Directory where calibration data is picked up" default:"calibration-data" value-name:"<DIR>"`

	// Verbose
	Verbose bool `short:"v" long:"verbose"`
}

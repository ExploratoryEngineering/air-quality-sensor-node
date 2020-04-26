package opts

// Opts contains command line options
type Opts struct {
	HordeCollection string `short:"c" long:"collection" description:"Horde collection id to listen to" default:"17dh0cf43jg007" value-name:"<collectionID>"`

	// Database options
	DBFilename string `short:"d" long:"db" description:"Data storage file" default:"aq.db" value-name:"<file>"`

	// Calibration data distribution URL pointing to S3 bucket
	CalDataS3URL string `long:"cal-data-s3-url" description:"S3 URL to where calibration data is published" default:"https://calibration-data.s3-eu-west-1.amazonaws.com/" value-name:"<URL>"`

	// Verbose
	Verbose bool `short:"v" long:"verbose"`
}

package opts

// Opts contains command line options
type Opts struct {
	HordeCollection string `short:"c" long:"collection" description:"Horde collection id to listen to" default:"17dh0cf43jg007" value-name:"<collectionID>"`

	// Database options
	DBFilename string `short:"d" long:"db" description:"Data storage file" default:"aq.db" value-name:"<file>"`

	// Verbose
	Verbose bool `short:"v" long:"verbose"`
}

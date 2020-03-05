package opts

import (
	"log"
	"os"

	"github.com/jessevdk/go-flags"
)

// Options struct
type Opts struct {
	// Webserver options
	WebServer string `short:"w" long:"webserver" description:"Listen address for webserver" default:":8888" value-name:"<[host]:port>"`

	// UDP listener
	UDPListenAddress string `short:"u" long:"udp-listener" description:"Listen address for UDP listener" default:":9191" value-name:"<[host]:port>"`

	// Database options
	DBFilename string `short:"d" long:"db" description:"Data storage file" default:"ds.db" value-name:"<file>"`
}

var parsedOpts Opts

// ParseOpts
func Parse() *Opts {
	parser := flags.NewParser(&parsedOpts, flags.Default)

	_, err := parser.Parse()
	if err != nil {
		if flagsErr, ok := err.(*flags.Error); ok && flagsErr.Type == flags.ErrHelp {
			os.Exit(0)
		}
		log.Fatalf("Error parsing flags: %v", err)
	}
	return &parsedOpts
}

// Opts returns the command line options for the server command
func POarsedOpts() *Opts {
	return &parsedOpts
}

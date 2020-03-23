package main

import (
	"os"

	"github.com/jessevdk/go-flags"
)

// Options defines the global command line parameters
type Options struct {
	DBFilename string `short:"d" long:"db" description:"Data storage file" default:"aq.db" value-name:"<file>"`
	Verbose    bool   `short:"v" long:"verbose" description:"Verbose output"`
}

const (
	defaultCollection = "17dh0cf43jg007"
)

var options Options
var parser = flags.NewParser(&options, flags.Default)

func main() {
	if _, err := parser.Parse(); err != nil {
		if flagsErr, ok := err.(*flags.Error); ok && flagsErr.Type == flags.ErrHelp {
			os.Exit(0)
		} else {
			os.Exit(1)
		}
	}
}

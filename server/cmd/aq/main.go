package main

import (
	"os"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/jessevdk/go-flags"
)

var options opts.Opts
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

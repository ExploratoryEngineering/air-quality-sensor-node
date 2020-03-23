package main

import "fmt"

// AddCommand defines the command line parameters for add command
type AddCommand struct {
	CollectionID string `short:"c" long:"collection" description:"ID of collection" value-name:"<id>" required:"true"`
	DeviceID     string `short:"d" long:"device" description:"ID of device" value-name:"<id>" required:"true"`
}

var addCommand AddCommand

func init() {
	parser.AddCommand("add",
		"Add a calibration data",
		"The add command adds a the calibration data to the database",
		&addCommand)
}

// Execute runs the add command
func (a *AddCommand) Execute(args []string) error {
	fmt.Printf("Adding collectionID = %s, device = %s: %#v\n", a.CollectionID, a.DeviceID, args)
	return nil
}

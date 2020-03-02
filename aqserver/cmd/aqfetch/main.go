//
// This utility connects to the server and receives messages for a
// given device in real-time, printing out the payload on stdout.
//
//
package main

import (
	"flag"
	"fmt"
	"io"
	"log"

	"github.com/telenordigital/nbiot-go"
)

var (
	collectionID = flag.String("collection", "17dh0cf43jg007", "Collection ID")
	deviceID     = flag.String("device", "17dh0cf43jg6n4", "Device ID")
)

func main() {
	flag.Parse()

	client, err := nbiot.New()
	if err != nil {
		log.Fatal("Error creating client:", err)
	}

	stream, err := client.DeviceOutputStream(*collectionID, *deviceID)
	if err != nil {
		log.Fatal("Error creating stream: ", err)
	}
	defer stream.Close()

	log.Printf("Listening for messages from device '%s' in collection '%s'", *deviceID, *collectionID)
	for {
		data, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatal("Error receiving data: ", err)
		}

		s := "{"

		for _, b := range data.Payload {
			s += fmt.Sprintf("0x%02x, ", b)
		}
		s += "}"

		log.Printf("received payload len=%d: %s", len(data.Payload), s)
	}

}

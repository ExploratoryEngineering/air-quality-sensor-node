//
// This utility connects to the server and receives messages for a
// given device in real-time, printing out the payload on stdout.
//
//
package main

import (
	"encoding/json"
	"flag"
	"io"
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/telenordigital/nbiot-go"
	"google.golang.org/protobuf/proto"
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

		sample := aqpb.Sample{}
		err = proto.Unmarshal(data.Payload, &sample)
		if err != nil {
			log.Printf("Unable to unmarshal protobuf payload: %v", err)
			continue
		}

		m := model.MessageFromProtobuf(&sample)
		if m == nil {
			log.Printf("Unable to create Message from protobuf")
			continue
		}

		json, _ := json.MarshalIndent(m, "", "\t")
		log.Printf("JSON:\n%s\n", json)
	}

}

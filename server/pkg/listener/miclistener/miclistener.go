package miclistener

import (
	"encoding/json"
	"fmt"
	"log"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/telenordigital/nbiot-go"
)

// MICListener connects to MIC and listens for messages from a
// particular Thing
type MICListener struct {
	pipeline pipeline.Pipeline
	doneChan chan error
	quit     chan bool
	client   *nbiot.Client
	opts     *opts.Opts
}

// credentials struct to parse JSON response from MIC login endpoint.
// The token is used to login to the AWS Cogntio service and generate
// access tokens used to pre-sign a websocket URL using AWS Signature V4.
// Weird and complicated, but necessary.
type credentials struct {
	Token      string `json:"token"`
	IdentityID string `json:"identityId"`
}

// mqttState struct for nested JSON object parsing in Go, wee.
type mqttState struct {
	State mqttReported `json:"state"`
}

// mqttReported struct for nested JSON object parsing in Go, wee.
type mqttReported struct {
	Reported mqttPayload `json:"reported"`
}

// mqttPayload struct for nested JSON object parsing in Go, wee.
type mqttPayload struct {
	Backdate int            `json:"backdate"`
	Raw      mqttPayloadRaw `json:"raw"`
}

// mqttPayloadRaw struct for nested JSON object parsing in Go, wee.
type mqttPayloadRaw struct {
	Data []byte `json:"data"`
}

// TODO(pontus): Find better placement for opts/configs.
const (
	micUsername    = "<redacted>"                                                  // not added
	micPassword    = "<redacted>"                                                  // not added
	micTopic       = "thing-update/StartIoT/trondheim.kommune.no/#"                // added
	awsAPIKey      = "3puriPZzDf9Mo664Oyuow1GO1B7TzX9J7oqxXISx"                    // not added
	awsAPIGateway  = "https://3ohe8pnzfb.execute-api.eu-west-1.amazonaws.com/prod" // added
	awsRegion      = "eu-west-1"                                                   // added
	awsUserPool    = "eu-west-1_wsOo2av1M"                                         // added
	awsIoTEndpoint = "a15nxxwvsld4o-ats"                                           // added
)

// New creates a new MICListener instance
func New(opts *opts.Opts, pipeline pipeline.Pipeline) *MICListener {
	return &MICListener{
		opts:     opts,
		pipeline: pipeline,
		doneChan: make(chan error),
		quit:     make(chan bool),
	}
}

// Start MICListener instance
func (h *MICListener) Start() error {
	credRes, err := micLogin()
	if err != nil {
		return err
	}

	wsURL := awsIotWsURL(
		*credRes.Credentials.AccessKeyId,
		*credRes.Credentials.SecretKey,
		*credRes.Credentials.SessionToken,
		awsRegion,
		awsIoTEndpoint,
	)

	opts := MQTT.NewClientOptions().AddBroker(wsURL)
	opts.SetClientID("mic-go-client" + time.Now().String())
	client := MQTT.NewClient(opts)

	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	} else {
		log.Printf("Connected to server\n")

		// Function invoked each time a new MQTT message is received
		var f MQTT.MessageHandler = func(client MQTT.Client, msg MQTT.Message) {
			var state mqttState
			if err := json.Unmarshal(msg.Payload(), &state); err != nil {
				log.Fatal(err)
			}

			if state.State.Reported.Backdate != 0 {
				return
			}

			protobufBytes := state.State.Reported.Raw
			fmt.Printf("%+v\n", protobufBytes)

			pb, err := model.ProtobufFromData(protobufBytes.Data)
			if err != nil {
				log.Printf("Failed to decode protobuffer len=%d: %v", len(protobufBytes.Data), err)
			}

			m := model.MessageFromProtobuf(pb)
			if m == nil {
				log.Printf("Unable to create Message from protobuf")
			}

			// TODO(pontus): unsure where deviceID can be found. If not in payload we don't have it :(
			// m.DeviceID = data.Device.ID
			// m.ReceivedTime = data.Received
			m.PacketSize = len(protobufBytes.Data)

			// TODO(borud): This is a good place to check if a device
			//    is already known and inject it into the database.

			if h.opts.Verbose {
				log.Printf("Accepted packet from Horde %v", m)
			}
			h.pipeline.Publish(m)
		}

		if token := client.Subscribe(micTopic, 0, f); token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
		} else {
			log.Printf("Subscribed to topic\n")
		}
	}

	return nil
}

// Shutdown initiates shutdown of the MICListener
func (h *MICListener) Shutdown() {
	log.Printf("miclistener: Shutdown not implemented")
}

// WaitForShutdown waits for the MICListener listener to shut down
func (h *MICListener) WaitForShutdown() {
	<-h.quit
}

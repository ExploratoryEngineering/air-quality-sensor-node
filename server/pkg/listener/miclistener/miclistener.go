package miclistener

import (
	"encoding/json"
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
	config   *Config
}

// Config contains the configuration for this listener
type Config struct {
	Username       string
	Password       string
	AWSAPIKey      string
	Topic          string
	AWSRegion      string
	AWSAPIGateway  string
	AWSUserPool    string
	AWSIoTEndpoint string
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

// New creates a new MICListener instance
func New(opts *opts.Opts, pipeline pipeline.Pipeline, config *Config) *MICListener {
	return &MICListener{
		opts:     opts,
		pipeline: pipeline,
		config:   config,
		doneChan: make(chan error),
		quit:     make(chan bool),
	}
}

// Start MICListener instance
func (h *MICListener) Start() error {
	credRes, err := h.micLogin()
	if err != nil {
		return err
	}

	wsURL := awsIotWsURL(
		*credRes.Credentials.AccessKeyId,
		*credRes.Credentials.SecretKey,
		*credRes.Credentials.SessionToken,
		h.config.AWSRegion,
		h.config.AWSIoTEndpoint,
	)

	opts := MQTT.NewClientOptions().AddBroker(wsURL)
	opts.SetClientID("mic-go-client" + time.Now().String())
	client := MQTT.NewClient(opts)

	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	} else {
		log.Printf("Connected to server: %s\n", h.config.AWSIoTEndpoint)

		// Function invoked each time a new MQTT message is received
		var f MQTT.MessageHandler = func(client MQTT.Client, msg MQTT.Message) {
			var state mqttState

			if err := json.Unmarshal(msg.Payload(), &state); err != nil {
				log.Printf("Unable to unmarshal MQTT payload: '%s': %v", msg.Payload(), err)
				return
			}

			// We get two messages.  One is the one we want to parse
			// that contains a protobuffer and the other is a
			// backdated message.
			if state.State.Reported.Backdate != 0 {
				return
			}

			protobufBytes := state.State.Reported.Raw
			pb, err := model.ProtobufFromData(protobufBytes.Data)
			if err != nil {
				log.Printf("Failed to decode protobuffer len=%d: %v", len(protobufBytes.Data), err)
				return
			}

			m := model.MessageFromProtobuf(pb)

			// We can't know the received time (also note that MIC has
			// pretty high latency and the latency is variable, so the
			// timestamp can be several seconds late.
			m.ReceivedTime = time.Now().UnixNano() / int64(time.Millisecond)

			m.PacketSize = len(protobufBytes.Data)
			if h.opts.Verbose {
				log.Printf("Accepted packet from Horde %v", m)
			}
			h.pipeline.Publish(m)
		}

		if token := client.Subscribe(h.config.Topic, 0, f); token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
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

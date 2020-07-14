package listener

import (
	"bytes"
	"crypto/hmac"
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/opts"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/cognitoidentity"
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

// Credentials struct to parse JSON response from MIC login endpoint.
// The token is used to login to the AWS Cogntio service and generate
// access tokens used to pre-sign a websocket URL using AWS Signature V4.
// Weird and complicated, but necessary.
type Credentials struct {
	Token      string `json:"token"`
	IdentityID string `json:"identityId"`
}

// MQTTState struct for nested JSON object parsing in Go, wee.
type MQTTState struct {
	State MQTTReported `json:"state"`
}

// MQTTReported struct for nested JSON object parsing in Go, wee.
type MQTTReported struct {
	Reported MQTTPayload `json:"reported"`
}

// MQTTPayload struct for nested JSON object parsing in Go, wee.
type MQTTPayload struct {
	Backdate int            `json:"backdate"`
	Raw      MQTTPayloadRaw `json:"raw"`
}

// MQTTPayloadRaw struct for nested JSON object parsing in Go, wee.
type MQTTPayloadRaw struct {
	Data []byte `json:"data"`
}

// TODO(pontus): Find better placement for opts/configs.
const (
	micUsername    = "<redacted>"
	micPassword    = "<redacted>"
	micTopic       = "thing-update/StartIoT/trondheim.kommune.no/#"
	awsAPIKey      = "3puriPZzDf9Mo664Oyuow1GO1B7TzX9J7oqxXISx"
	awsAPIGateway  = "https://3ohe8pnzfb.execute-api.eu-west-1.amazonaws.com/prod"
	awsRegion      = "eu-west-1"
	awsUserPool    = "eu-west-1_wsOo2av1M"
	awsIoTEndpoint = "a15nxxwvsld4o-ats"
)

// Below follows six functions to handle AWS Signature V4 manually since
// AWS SDK's doesn't handle the signing correctly for the "iotdevicegateway"-service.
// References:
// 		https://github.com/aws/aws-sdk-go/issues/820
// 		https://github.com/aws/aws-sdk-go/issues/706
func micLogin() (*cognitoidentity.GetCredentialsForIdentityOutput, error) {
	reqBody, err := json.Marshal(map[string]string{
		"userName": micUsername,
		"password": micPassword,
	})
	if err != nil {
		return nil, err
	}

	client := http.Client{}
	request, err := http.NewRequest("POST", awsAPIGateway+"/auth/login", bytes.NewBuffer(reqBody))
	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", awsAPIKey)
	if err != nil {
		return nil, err
	}

	resp, err := client.Do(request)
	if err != nil {
		return nil, err
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var data map[string]Credentials
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		return nil, err
	}

	token := data["credentials"].Token
	identityID := data["credentials"].IdentityID

	ses, _ := session.NewSession(&aws.Config{Region: aws.String(awsRegion)})
	svc := cognitoidentity.New(ses)
	credRes, err := svc.GetCredentialsForIdentity(&cognitoidentity.GetCredentialsForIdentityInput{
		IdentityId: aws.String(identityID),
		Logins: map[string]*string{
			"cognito-idp." + awsRegion + ".amazonaws.com/" + awsUserPool: &token,
		},
	})

	return credRes, err
}

func awsIotWsURL(accessKey string, secretKey string, sessionToken string, region string, endpoint string) string {
	host := fmt.Sprintf("%s.iot.%s.amazonaws.com", endpoint, region)

	// According to docs, time must be within 5min of actual time (or at least according to AWS servers)
	now := time.Now().UTC()

	dateLong := now.Format("20060102T150405Z")
	dateShort := dateLong[:8]
	serviceName := "iotdevicegateway"
	scope := fmt.Sprintf("%s/%s/%s/aws4_request", dateShort, region, serviceName)
	alg := "AWS4-HMAC-SHA256"
	q := [][2]string{
		{"X-Amz-Algorithm", alg},
		{"X-Amz-Credential", accessKey + "/" + scope},
		{"X-Amz-Date", dateLong},
		{"X-Amz-SignedHeaders", "host"},
	}

	query := awsQueryParams(q)

	signKey := awsSignKey(secretKey, dateShort, region, serviceName)
	stringToSign := awsSignString(accessKey, secretKey, query, host, dateLong, alg, scope)
	signature := fmt.Sprintf("%x", awsHmac(signKey, []byte(stringToSign)))

	wsurl := fmt.Sprintf("wss://%s/mqtt?%s&X-Amz-Signature=%s", host, query, signature)

	if sessionToken != "" {
		wsurl = fmt.Sprintf("%s&X-Amz-Security-Token=%s", wsurl, url.QueryEscape(sessionToken))
	}

	return wsurl
}

func awsQueryParams(q [][2]string) string {
	var buff bytes.Buffer
	var i int
	for _, param := range q {
		if i != 0 {
			buff.WriteRune('&')
		}
		i++
		buff.WriteString(param[0])
		buff.WriteRune('=')
		buff.WriteString(url.QueryEscape(param[1]))
	}
	return buff.String()
}

func awsSignString(accessKey string, secretKey string, query string, host string, dateLongStr string, alg string, scopeStr string) string {
	emptyStringHash := "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
	req := strings.Join([]string{
		"GET",
		"/mqtt",
		query,
		"host:" + host,
		"", // separator
		"host",
		emptyStringHash,
	}, "\n")
	return strings.Join([]string{
		alg,
		dateLongStr,
		scopeStr,
		awsSha(req),
	}, "\n")
}

func awsHmac(key []byte, data []byte) []byte {
	h := hmac.New(sha256.New, key)
	h.Write(data)
	return h.Sum(nil)
}

func awsSignKey(secretKey string, dateShort string, region string, serviceName string) []byte {
	h := awsHmac([]byte("AWS4"+secretKey), []byte(dateShort))
	h = awsHmac(h, []byte(region))
	h = awsHmac(h, []byte(serviceName))
	h = awsHmac(h, []byte("aws4_request"))
	return h
}

func awsSha(in string) string {
	h := sha256.New()
	fmt.Fprintf(h, "%s", in)
	return fmt.Sprintf("%x", h.Sum(nil))
}

// NewMICListener creates a new MICListener instance
func NewMICListener(opts *opts.Opts, pipeline pipeline.Pipeline) *MICListener {
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
			var state MQTTState
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
	log.Printf("MICListener: Shutdown not implemented")
}

// WaitForShutdown waits for the MICListener listener to shut down
func (h *MICListener) WaitForShutdown() {
	<-h.quit
}

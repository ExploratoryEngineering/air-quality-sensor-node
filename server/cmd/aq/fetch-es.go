package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/calculate"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/persist"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store/sqlitestore"
)

// Credentials ...
type Credentials struct {
	Token      string `json:"token"`
	IdentityID string `json:"identityId"`
}

// ESResult ...
type ESResult struct {
	Shards ESShard `json:"_shards"`
	Hits   ESHits  `json:"hits"`
}

// ESShard ...
type ESShard struct {
	Total      int `json:"total"`
	Successful int `json:"successful"`
	Failed     int `json:"failed"`
}

// ESHits ...
type ESHits struct {
	Total int     `json:"total"`
	Hits  []ESHit `json:"hits"`
}

// ESHit ...
type ESHit struct {
	Source ESSource `json:"_source"`
}

// ESSource ...
type ESSource struct {
	Timestamp int     `json:"timestamp"`
	ThingName string  `json:"thingName"`
	ThingType string  `json:"thingType"`
	State     ESState `json:"state"`
}

// ESState ...
type ESState struct {
	Backdate int          `json:"backdate"`
	Raw      ESPayloadRaw `json:"raw"`
}

// ESPayloadRaw ...
type ESPayloadRaw struct {
	Data []byte `json:"data"`
}

// FetchCommand ...
type FetchCommand struct {
	PageSize int `short:"p" long:"page-size" description:"Number of rows to fetch per page" default:"250"`
}

// For this application we say that time begins on 2020-03-25
var beginningOfTime = int64(1585094400000)

const (
	micUsername   = "<redacted>"
	micPassword   = "<redacted>"
	micThingType  = 191
	awsAPIKey     = "3puriPZzDf9Mo664Oyuow1GO1B7TzX9J7oqxXISx"
	awsAPIGateway = "https://3ohe8pnzfb.execute-api.eu-west-1.amazonaws.com/prod"
)

func micLogin() (*string, error) {
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

	return &token, nil
}

func fetchPage(token *string, pageSize int, gte int64, lte int64) ([]ESHit, error) {
	query := fmt.Sprintf(`
	{
		"queryScope": {
			"thingTypes": ["%d"]
		},
		"query": {
			"size": %d,
			"track_scores": 0,
			"query": {
				"bool": {
					"filter": {
						"bool": {
							"must": [
								{
									"term": {
										"state.backdate": "0"
									}
								},
                {
									"range": {
										"timestamp": {
											"gte": "%d",
											"lte": "%d"
										}
									}
								}
							]
						}
					}
				}
			}
		}
	}
	`, micThingType, pageSize, gte, lte)

	client := http.Client{}
	request, err := http.NewRequest("POST", awsAPIGateway+"/observations/find", bytes.NewBufferString(query))
	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", awsAPIKey)
	request.Header.Set("Authorization", *token)
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

	var data ESResult
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		return nil, err
	}

	return data.Hits.Hits, nil
}

func init() {
	parser.AddCommand(
		"fetch",
		"Fetch historical data",
		"Fetch historical sensor data from MIC Elasticsearch index",
		&FetchCommand{})
}

// Execute ...
func (a *FetchCommand) Execute(args []string) error {
	token, err := micLogin()
	if err != nil {
		return err
	}

	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", options.DBFilename, err)
	}
	defer db.Close()

	// Make sure we have latest calibration data before fetching
	err = checkForNewCalibrationData(db)
	if err != nil {
		log.Printf("Unable to download calibration data: %v", err)
	}

	data, err := db.ListMessages(0, 1)
	if err != nil {
		log.Fatalf("Unable to list messages: %v", err)
	}

	if len(data) == 1 {
		// I'm assuming we have to add an entire second of data here
		// in order to make up for the API not having millisecond
		// resolution?
		beginningOfTime = data[0].ReceivedTime + 1
		log.Printf("Will fetch back to %s", msToTime(beginningOfTime))
	}

	// Set up pipeline
	pipelineRoot := pipeline.New(&options, db)
	pipelineCalc := calculate.New(&options, db)
	pipelinePersist := persist.New(&options, db)

	pipelineRoot.AddNext(pipelineCalc)
	pipelineCalc.AddNext(pipelinePersist)

	var since = beginningOfTime
	var until = time.Now().UnixNano() / int64(time.Millisecond)
	var count = 0
	var countTotal = 0

	for {
		data, err := fetchPage(token, a.PageSize, since, until)
		if err != nil {
			log.Fatalf("Error while reading data: %v", err)
		}

		if len(data) == 0 {
			break
		}

		for _, d := range data {
			protobufBytes := d.Source.State.Raw

			pb, err := model.ProtobufFromData(protobufBytes.Data)
			if err != nil {
				log.Printf("Failed to decode protobuffer len=%d: %v", len(protobufBytes.Data), err)
				continue
			}

			m := model.MessageFromProtobuf(pb)
			if m == nil {
				log.Printf("Unable to create Message from protobuf")
				continue
			}

			// TODO(pontus): unsure where deviceID can be found. If not in payload we don't have it :(
			// m.DeviceID = d.Device.ID
			// m.ReceivedTime = d.Received
			m.PacketSize = len(protobufBytes.Data)

			pipelineRoot.Publish(m)
			count++
			countTotal++
		}

		since = int64(data[len(data)-1].Source.Timestamp + 1)
		until = time.Now().UnixNano() / int64(time.Millisecond)
		if count >= 500 {
			log.Printf("Imported %d records...", countTotal)
			count = 0
		}
	}

	log.Printf("Fetched a total of %d messages", countTotal)
	return nil
}

// msToTime converts milliseconds since epoch to time.Time
func msToTime(t int64) time.Time {
	return time.Unix(t/int64(1000), (t%int64(1000))*int64(1000000))
}

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

type esResult struct {
	Shards esShard `json:"_shards"`
	Hits   esHits  `json:"hits"`
}
type esShard struct {
	Total      int `json:"total"`
	Successful int `json:"successful"`
	Failed     int `json:"failed"`
}

type esHits struct {
	Total int     `json:"total"`
	Hits  []esHit `json:"hits"`
}

type esHit struct {
	Source esSource `json:"_source"`
}

type esSource struct {
	Timestamp int     `json:"timestamp"`
	ThingName string  `json:"thingName"`
	ThingType string  `json:"thingType"`
	State     esState `json:"state"`
}

type esState struct {
	Backdate int          `json:"backdate"`
	Raw      esPayloadRaw `json:"raw"`
}

// ESPayloadRaw ...
type esPayloadRaw struct {
	Data []byte `json:"data"`
}

// FetchESCommand ...
type FetchESCommand struct {
	PageSize int `short:"p" long:"page-size" description:"Number of rows to fetch per page" default:"250"`
}

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

func fetchPage(token *string, pageSize int, gte int64, lte int64) ([]esHit, error) {
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
	if err != nil {
		return nil, err
	}

	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", awsAPIKey)
	request.Header.Set("Authorization", *token)

	resp, err := client.Do(request)
	if err != nil {
		return nil, err
	}

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("Failed to fetch data: %s", resp.Status)
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var data esResult
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		return nil, err
	}

	return data.Hits.Hits, nil
}

func init() {
	parser.AddCommand(
		"fetch-mic",
		"Fetch historical data",
		"Fetch historical sensor data from MIC Elasticsearch index",
		&FetchESCommand{})
}

// Execute ...
func (a *FetchESCommand) Execute(args []string) error {
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

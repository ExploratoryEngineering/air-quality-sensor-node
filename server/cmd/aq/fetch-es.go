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

type credentials struct {
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
	PageSize         int    `short:"p" long:"page-size" description:"Number of rows to fetch per page" default:"250"`
	MICUsername      string `long:"mic-username" env:"MIC_USERNAME" description:"MIC Username" default:""`
	MICPassword      string `long:"mic-password" env:"MIC_PASSWORD" description:"MIC Username" default:""`
	MICAWSAPIKey     string `long:"mic-api-key" env:"MIC_AWS_API_KEY" description:"MIC Username" default:""`
	MICAWSRegion     string `long:"mic-aws-region" description:"AWS region for MIC" default:"eu-west-1"`
	MICAWSAPIGateway string `long:"mic-aws-api-gw" description:"AWS API gateway" default:"https://3ohe8pnzfb.execute-api.eu-west-1.amazonaws.com/prod"`
	MICThingType     int    `long:"mic-thing-type" description:"MIC thing type" default:"191"`
}

func (a *FetchESCommand) micLogin() (*string, error) {
	reqBody, err := json.Marshal(map[string]string{
		"userName": a.MICUsername,
		"password": a.MICPassword,
	})
	if err != nil {
		return nil, err
	}

	log.Printf("ReqBody: \n%s\n", reqBody)

	client := http.Client{}
	request, err := http.NewRequest("POST", a.MICAWSAPIGateway+"/auth/login", bytes.NewBuffer(reqBody))
	if err != nil {
		return nil, err
	}

	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", a.MICAWSAPIKey)

	resp, err := client.Do(request)
	if err != nil {
		return nil, err
	}
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("Failed to log into MIC: %s", resp.Status)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var data map[string]credentials
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		return nil, err
	}

	token := data["credentials"].Token

	return &token, nil
}

func (a *FetchESCommand) fetchPage(token *string, pageSize int, gte int64, lte int64) ([]esHit, error) {
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
	`, a.MICThingType, pageSize, gte, lte)

	client := http.Client{}
	request, err := http.NewRequest("POST", a.MICAWSAPIGateway+"/observations/find", bytes.NewBufferString(query))
	if err != nil {
		return nil, err
	}

	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", a.MICAWSAPIKey)
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
		"fetch",
		"Fetch historical data",
		"Fetch historical sensor data from MIC Elasticsearch index",
		&FetchESCommand{})
}

// Execute ...
func (a *FetchESCommand) Execute(args []string) error {
	token, err := a.micLogin()
	if err != nil {
		return err
	}

	db, err := sqlitestore.New(options.DBFilename)
	if err != nil {
		log.Fatalf("Unable to open or create database file '%s': %v", options.DBFilename, err)
	}
	defer db.Close()

	// Make sure we have latest calibration data before fetching
	loadCalibrationData(db, options.CalibrationDataDir)

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
		data, err := a.fetchPage(token, a.PageSize, since, until)
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

			m.ReceivedTime = int64(d.Source.Timestamp)
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

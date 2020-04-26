package caldata

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/url"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// DownloadFromS3 downloads all the calibration data from S3 and
// returns it.  If it encounters an error anywhere along the way it
// will return the underlying error.
//
func (c *Caldata) DownloadFromS3() ([]model.Cal, error) {
	// Fetch list of files
	resp, err := http.Get(c.baseURL.String())
	if err != nil {
		return nil, err
	}

	// Fetch the body and parse it into a list of calibration data files
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	names, err := parseS3BucketListing(body)
	if err != nil {
		return nil, err
	}

	cals := make([]model.Cal, len(names))

	// Download all the files and parse them
	for n, name := range names {
		u := url.URL{
			Scheme: c.baseURL.Scheme,
			Host:   c.baseURL.Host,
			Path:   url.PathEscape(name),
		}

		resp, err := http.Get(u.String())
		if err != nil {
			return nil, err
		}

		fileBody, err := ioutil.ReadAll(resp.Body)
		resp.Body.Close()

		err = json.Unmarshal(fileBody, &cals[n])
		if err != nil {
			return nil, err
		}
	}

	return cals, nil
}

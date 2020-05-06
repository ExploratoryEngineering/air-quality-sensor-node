package caldata

import "net/url"

// Caldata ...
type Caldata struct {
	baseURL *url.URL
}

// NewCaldata creates a new instance of Caldata
func NewCaldata(s3URL string) (*Caldata, error) {
	u, err := url.Parse(s3URL)
	if err != nil {
		return nil, err
	}

	return &Caldata{
		baseURL: u,
	}, nil
}

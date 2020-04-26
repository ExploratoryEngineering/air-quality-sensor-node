package caldata

import (
	"log"
	"testing"

	"github.com/stretchr/testify/assert"
)

const baseURL = "https://calibration-data.s3-eu-west-1.amazonaws.com/"

func TestCaldata(t *testing.T) {
	cd, err := NewCaldata(baseURL)
	assert.Nil(t, err)
	assert.NotNil(t, cd)

	cals, err := cd.DownloadFromS3()
	assert.Nil(t, err)
	assert.NotNil(t, cals)

	for _, cal := range cals {
		log.Printf("%s %s %s", cal.DeviceID, cal.CollectionID, cal.ValidFrom)
	}

}

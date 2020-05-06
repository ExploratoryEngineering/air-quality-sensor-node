package caldata

import (
	"encoding/xml"
	"time"
)

type listBucketResult struct {
	XMLName     string     `xml:"ListBucketResult"`
	Name        string     `xml:"Name"`
	Prefix      string     `xml:"Prefix"`
	Marker      string     `xml:"Marker"`
	MaxKeys     int        `xml:"MaxKeys"`
	IsTruncated bool       `xml:"IsTruncated"`
	Contents    []contents `xml:"Contents"`
}

type contents struct {
	XMLName      string    `xml:"Contents"`
	Key          string    `xml:"Key"`
	LastModified time.Time `xml:"LastModified"`
	ETag         string    `xml:"ETag"`
	Size         int       `xml:"Size"`
	StorageClass string    `xml:"StorageClass"`
}

func parseS3BucketListing(data []byte) ([]string, error) {
	var lbr listBucketResult
	err := xml.Unmarshal(data, &lbr)
	if err != nil {
		return nil, err
	}

	names := make([]string, len(lbr.Contents))
	for i := 0; i < len(lbr.Contents); i++ {
		names[i] = lbr.Contents[i].Key
	}

	return names, nil
}

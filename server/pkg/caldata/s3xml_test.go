package caldata

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
)

var data = []byte(`
<ListBucketResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
  <Name>calibration-data</Name>
  <Prefix/>
  <Marker/>
  <MaxKeys>1000</MaxKeys>
  <IsTruncated>false</IsTruncated>
  <Contents>
	<Key>16-000160.json</Key>
	<LastModified>2020-04-26T15:37:49.000Z</LastModified>
	<ETag>"fb79c5cc2e4d701a4ac2b98872205053"</ETag>
	<Size>849</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
  <Contents>
	<Key>16-000164.json</Key>
	<LastModified>2020-04-26T15:37:49.000Z</LastModified>
	<ETag>"fde9e9e94fa9971761df81ecacfb41df"</ETag>
	<Size>848</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
  <Contents>
	<Key>16-000167.json</Key>
	<LastModified>2020-04-26T15:37:49.000Z</LastModified>
	<ETag>"4c56940315d58b2b8bf3f185a7ab8394"</ETag>
	<Size>846</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
  <Contents>
	<Key>16-000170.json</Key>
	<LastModified>2020-04-26T15:37:49.000Z</LastModified>
	<ETag>"5a4a37b016f311819e4f92605cc0988c"</ETag>
	<Size>848</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
  <Contents>
	<Key>16-000171.json</Key>
	<LastModified>2020-04-26T15:37:49.000Z</LastModified>
	<ETag>"ed98fe915752dc01805af86bee9ccd55"</ETag>
	<Size>847</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
  <Contents>
	<Key>16-000172.json</Key>
	<LastModified>2020-04-26T15:37:50.000Z</LastModified>
	<ETag>"ba40fdcbeb6e35f4d1620dca7c98472e"</ETag>
	<Size>848</Size>
	<StorageClass>STANDARD</StorageClass>
  </Contents>
</ListBucketResult>
`)

func TestParseS3BucketListing(t *testing.T) {
	names, err := parseS3BucketListing(data)
	assert.Nil(t, err)
	assert.Equal(t, 6, len(names))

	sort.Strings(names)
	assert.Equal(t, "16-000160.json", names[0])
	assert.Equal(t, "16-000164.json", names[1])
	assert.Equal(t, "16-000167.json", names[2])
	assert.Equal(t, "16-000170.json", names[3])
	assert.Equal(t, "16-000171.json", names[4])
	assert.Equal(t, "16-000172.json", names[5])

}

package codec

import (
	"encoding/json"
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

var testdata = []byte{
	0x00, 0x00, 0x00, 0x00, // float32 - GPS Timestamp, seconds since epoch
	0x00, 0x00, 0x00, 0x00, // float32 - GPS longitude - radians
	0x00, 0x00, 0x00, 0x00, // float32 - GPS latitude -radians
	0x00, 0x00, 0x00, 0x00, // float32 - GPS altitude, meters
	0x00, 0x00, 0x00, 0x7c, // float32 - Board temperature, celsius
	0x00, 0x00, 0x00, 0x63, // float32 - Board relative humidity, percent
	0x00, 0x07, 0xe6, 0x0e, // uint32  - OP1 ADC reading - NO2 working electrode
	0x00, 0x07, 0xfd, 0x73, // uint32  - OP1 ADC reading - NO2 auxillary electrode
	0x00, 0x0a, 0x89, 0xa4, // uint32  - OP2 ADC reading - O3 + NO2 working electrode
	0x00, 0x0a, 0x87, 0xa1, // uint32  - OP2 ADC reading - O3 + NO2 auxillary electrode
	0x00, 0x05, 0x8d, 0x49, // uint32  - OP3 ADC reading - NO working electrode
	0x00, 0x05, 0x31, 0xd4, // uint32  - OP3 ADC reading - NO auxillary electrode
	0x00, 0x08, 0x52, 0xdc, // uint32  - Pt1000 ADC reading - AFE-3 ambient temperature
	0x00, 0x00, 0x00, 0x00, // uint32  - OPC PM A (default PM1)
	0x00, 0x00, 0x00, 0x00, // uint32  - OPC PM B (default PM2.5)
	0x00, 0x00, 0x00, 0x00, // uint32  - OPC PM C (default PM10)
	0x0c, 0x11, // uint16 - OPC sample period
	0x02, 0x20, // uint16 - OPC sample flowrate
	0x00, 0x1c, // uint16 - OPC temperature
	0x00, 0x00, // uint16 - OPC humidity
	0x02, 0x69, // uint16 - OPC fan rev count
	0x10, 0xe8, // uint16 - OPC laser status
	0x00, 0x02, // uint16 - OPC PM bin 1
	0x00, 0x00, // uint16 - OPC PM bin 2
	0x00, 0x00, // uint16 - OPC PM bin 3
	0x00, 0x00, // uint16 - OPC PM bin 4
	0x00, 0x00, // uint16 - OPC PM bin 5
	0x00, 0x00, // uint16 - OPC PM bin 6
	0x00, 0x00, // uint16 - OPC PM bin 7
	0x00, 0x00, // uint16 - OPC PM bin 8
	0x00, 0x00, // uint16 - OPC PM bin 9
	0x00, 0x00, // uint16 - OPC PM bin 10
	0x00, 0x00, // uint16 - OPC PM bin 11
	0x00, 0x00, // uint16 - OPC PM bin 12
	0x00, 0x00, // uint16 - OPC PM bin 13
	0x00, 0x00, // uint16 - OPC PM bin 14
	0x00, 0x00, // uint16 - OPC PM bin 15
	0x00, 0x00, // uint16 - OPC PM bin 16
	0x00, 0x00, // uint16 - OPC PM bin 17
	0x00, 0x00, // uint16 - OPC PM bin 18
	0x00, 0x00, // uint16 - OPC PM bin 19
	0x00, 0x00, // uint16 - OPC PM bin 20
	0x00, 0x00, // uint16 - OPC PM bin 21
	0x00, 0x00, // uint16 - OPC PM bin 22
	0x00, 0x00, // uint16 - OPC PM bin 23
	0x00, // uint8 - OPC sample valid
}

func TestDecode(t *testing.T) {
	dp, err := decode(testdata)
	assert.Nil(t, err)
	s, err := json.MarshalIndent(dp, "", "\t")
	assert.Nil(t, err)
	fmt.Printf("%s", s)
}

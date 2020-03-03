package codec

import (
	"bytes"
	"encoding/binary"
)

// DataPoint contains data from air quality sensor.
type DataPoint struct {
	GPSTime float32 `json:"timestamp"` // GPS Timestamp, seconds since epoch
	Long    float32 `json:"long"`      // Longitude in radians
	Lat     float32 `json:"lat"`       // Latitude in radians
	Alt     float32 `json:"alt"`       // Altitude in meters

	BoardTemp        float32 `json:"boardTemp"`        // Board Temperature, celsius
	BoardRelHumidity float32 `json:"boardRelHumidity"` // Board relative Humidity, percent

	// AFE3
	Sensor1Work uint32 `json:"Sensor1Work"` // OP1 ADC reading - NO2 working electrode
	Sensor1Aux  uint32 `json:"Sensor1Aux"`  // OP2 ADC reading - NO2 auxillary electrode
	Sensor2Work uint32 `json:"Sensor2Work"` // OP3 ADC reading - O3+NO2 working electrode
	Sensor2Aux  uint32 `json:"Sensor2Aux"`  // OP4 ADC reading - O3+NO2 auxillary electrode
	Sensor3Work uint32 `json:"Sensor3Work"` // OP5 ADC reading - NO working electrode
	Sensor32Aux uint32 `json:"Sensor3Aux"`  // OP6 ADC reading - NO aux electrode
	AFE3Temp    uint32 `json:"AFE3Temp"`    // Pt1000 ADC reading - AFE-3 ambient temperature

	// OPC-N3
	OPCPMA            uint32 `json:"OPCpmA"`          // OPC PM A (default PM1)
	OPCPMB            uint32 `json:"OPCpmB"`          // OPC PM B (default PM2.5)
	OPCPMC            uint32 `json:"OPCpmC"`          // OPC PM C (default PM10)
	OPCSamplePeriod   uint16 `json:"OPCSamplePeriod"` // OPC sample period, in ms
	OPCSampleFlowRate uint16 `json:"OPCFlowRate"`     // OPC sample flowrate, in ???
	OPCTemp           uint16 `json:"OPCTemp"`         // OPC temperature, in C (???)
	OPCFanRevCount    uint16 `json:"OPCFanRevCount"`  // OPC fan rev count
	OPCLaserStatus    uint16 `json:"OPCLaserStatus"`  // OPC laser status

	OPCBin1  uint16 `json:"OPCbin1"`  // OPC PM bin 1
	OPCBin2  uint16 `json:"OPCbin2"`  // OPC PM bin 2
	OPCBin3  uint16 `json:"OPCbin3"`  // OPC PM bin 3
	OPCBin4  uint16 `json:"OPCbin4"`  // OPC PM bin 4
	OPCBin5  uint16 `json:"OPCbin5"`  // OPC PM bin 5
	OPCBin6  uint16 `json:"OPCbin6"`  // OPC PM bin 6
	OPCBin7  uint16 `json:"OPCbin7"`  // OPC PM bin 7
	OPCBin8  uint16 `json:"OPCbin8"`  // OPC PM bin 8
	OPCBin9  uint16 `json:"OPCbin9"`  // OPC PM bin 9
	OPCBin10 uint16 `json:"OPCbin10"` // OPC PM bin 10
	OPCBin11 uint16 `json:"OPCbin11"` // OPC PM bin 11
	OPCBin12 uint16 `json:"OPCbin12"` // OPC PM bin 12
	OPCBin13 uint16 `json:"OPCbin13"` // OPC PM bin 13
	OPCBin14 uint16 `json:"OPCbin14"` // OPC PM bin 14
	OPCBin15 uint16 `json:"OPCbin15"` // OPC PM bin 15
	OPCBin16 uint16 `json:"OPCbin16"` // OPC PM bin 16
	OPCBin17 uint16 `json:"OPCbin17"` // OPC PM bin 17
	OPCBin18 uint16 `json:"OPCbin18"` // OPC PM bin 18
	OPCBin19 uint16 `json:"OPCbin19"` // OPC PM bin 19
	OPCBin20 uint16 `json:"OPCbin20"` // OPC PM bin 20
	OPCBin21 uint16 `json:"OPCbin21"` // OPC PM bin 21
	OPCBin22 uint16 `json:"OPCbin22"` // OPC PM bin 22
	OPCBin23 uint16 `json:"OPCbin23"` // OPC PM bin 23
	OPCBin24 uint16 `json:"OPCbin24"` // OPC PM bin 24

	OPCSampleValid uint8 `json:"sampleValid"` // OPC Sample valid
	Uptime         int64 `json:"Uptime"`      // Uptime
}

// DecodeAQMessage decodes the binary message from the AQ unit and
// populates a DataPoint struct with the values.  Note that this is
// very fragile when changes are made since there is no versioning of
// the protocol.
func DecodeAQMessage(data []byte) (DataPoint, error) {
	dp := DataPoint{}
	return dp, binary.Read(bytes.NewReader(data), binary.BigEndian, &dp)
}

package model

import (
	"time"
)

// DataPoint contains data from air quality sensor.  This type is part
// of the API so this is what the protobuffer gets translated into.
// This way we decouple the protobuffer datatype from the internal
// representation.
//
type DataPoint struct {
	// Board fields
	SysID            uint64  `json:"sysID"`            // System id, CPU id or similar
	FirmwareVersion  uint64  `json:"firmwareVersion"`  // Firmware version
	Uptime           int64   `json:"uptime"`           // Number of milliseconds since boot
	BoardTemp        float32 `json:"boardTemp"`        // Board Temperature, celsius
	BoardRelHumidity float32 `json:"boardRelHumidity"` // Board relative Humidity, percent
	Status           uint64  `json:"status"`           // Generic status bit field (for future use)

	// GPS fields
	GPSTimeStamp float32 `json:"gpsTimestamp"` // GPS Timestamp, seconds since epoch
	Lon          float32 `json:"long"`         // Longitude in radians
	Lat          float32 `json:"lat"`          // Latitude in radians
	Alt          float32 `json:"alt"`          // Altitude in meters

	// AFE3 fields
	Sensor1Work uint32 `json:"Sensor1Work"` // OP1 ADC reading - NO2 working electrode
	Sensor1Aux  uint32 `json:"Sensor1Aux"`  // OP2 ADC reading - NO2 auxillary electrode
	Sensor2Work uint32 `json:"Sensor2Work"` // OP3 ADC reading - O3+NO2 working electrode
	Sensor2Aux  uint32 `json:"Sensor2Aux"`  // OP4 ADC reading - O3+NO2 auxillary electrode
	Sensor3Work uint32 `json:"Sensor3Work"` // OP5 ADC reading - NO working electrode
	Sensor32Aux uint32 `json:"Sensor3Aux"`  // OP6 ADC reading - NO aux electrode
	AFE3Temp    uint32 `json:"AFE3Temp"`    // Pt1000 ADC reading - AFE-3 ambient temperature

	// OPC-N3
	OPCPMA            uint32     `json:"OPCpmA"`          // OPC PM A (default PM1)
	OPCPMB            uint32     `json:"OPCpmB"`          // OPC PM B (default PM2.5)
	OPCPMC            uint32     `json:"OPCpmC"`          // OPC PM C (default PM10)
	OPCSamplePeriod   uint16     `json:"OPCSamplePeriod"` // OPC sample period, in ms
	OPCSampleFlowRate uint16     `json:"OPCFlowRate"`     // OPC sample flowrate, in ???
	OPCTemp           uint16     `json:"OPCTemp"`         // OPC temperature, in C (???)
	OPCHum            uint16     `json:"OPCHum"`          // OPC humidity in percent
	OPCFanRevcount    uint16     `json:"OPCFanRevCount"`  // OPC fan rev count
	OPCLaserStatus    uint16     `json:"OPCLaserStatus"`  // OPC laser status
	OPCBins           [24]uint16 `json:"OPCBins"`         // OPC PM bins 0-23
	OPCSampleValid    uint8      `json:"sampleValid"`     // OPC Sample valid
}

// Message represents a message that has been received from the
// network.  Typically messages that come from Horde.
type Message struct {
	DeviceID     string
	CollectionID string
	ReceivedTime time.Time
	PacketSize   int
	DataPoint    *DataPoint
}

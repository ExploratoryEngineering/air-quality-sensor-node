package model

import (
	"time"
)

// Message contains data from air quality sensor.  This type is part
// of the API so this is what the protobuffer gets translated into.
// This way we decouple the protobuffer datatype from the internal
// representation.
//
type Message struct {
	// Housekeeping
	ID           int64     `db:"id" json:"id"`
	DeviceID     string    `db:"device_id" json:"deviceID"`
	ReceivedTime time.Time `db:"received" json:"receivedTime"`
	PacketSize   int       `db:"packetsize" json:"packetSize"`

	// Board fields
	SysID            uint64  `db:"sysid" json:"sysID"`                  // System id, CPU id or similar
	FirmwareVersion  uint64  `db:"firmware_ver" json:"firmwareVersion"` // Firmware version
	Uptime           int64   `db:"uptime" json:"uptime"`                // Number of milliseconds since boot
	BoardTemp        float32 `db:"boardtemp" json:"boardTemp"`          // Board Temperature, celsius
	BoardRelHumidity float32 `db:"boardhum" json:"boardRelHumidity"`    // Board relative Humidity, percent
	Status           uint64  `db:"status" json:"status"`                // Generic status bit field (for future use)

	// GPS fields
	GPSTimeStamp float32 `db:"gpstimestamp" json:"gpsTimestamp"` // GPS Timestamp, seconds since epoch
	Lon          float32 `db:"lon" json:"long"`                  // Longitude in radians
	Lat          float32 `db:"lat" json:"lat"`                   // Latitude in radians
	Alt          float32 `db:"alt" json:"alt"`                   // Altitude in meters

	// AFE3 fields
	Sensor1Work uint32 `db:"sensor1work" json:"Sensor1Work"` // OP1 ADC reading - NO2 working electrode
	Sensor1Aux  uint32 `db:"sensor1aux" json:"Sensor1Aux"`   // OP2 ADC reading - NO2 auxillary electrode
	Sensor2Work uint32 `db:"sensor2work" json:"Sensor2Work"` // OP3 ADC reading - O3+NO2 working electrode
	Sensor2Aux  uint32 `db:"sensor2aux" json:"Sensor2Aux"`   // OP4 ADC reading - O3+NO2 auxillary electrode
	Sensor3Work uint32 `db:"sensor3work" json:"Sensor3Work"` // OP5 ADC reading - NO working electrode
	Sensor32Aux uint32 `db:"sensor3aux" json:"Sensor3Aux"`   // OP6 ADC reading - NO aux electrode
	AFE3Temp    uint32 `db:"afe3temp" json:"AFE3Temp"`       // Pt1000 ADC reading - AFE-3 ambient temperature

	// OPC-N3
	OPCPMA            uint32 `db:"opcpma" json:"OPCpmA"`                   // OPC PM A (default PM1)
	OPCPMB            uint32 `db:"opcpmb" json:"OPCpmB"`                   // OPC PM B (default PM2.5)
	OPCPMC            uint32 `db:"opcpmc" json:"OPCpmC"`                   // OPC PM C (default PM10)
	OPCSamplePeriod   uint16 `db:"opcsampleperiod" json:"OPCSamplePeriod"` // OPC sample period, in ms
	OPCSampleFlowRate uint16 `db:"opcsampleflowrate" json:"OPCFlowRate"`   // OPC sample flowrate, in ???
	OPCTemp           uint16 `db:"opctemp" json:"OPCTemp"`                 // OPC temperature, in C (???)
	OPCHum            uint16 `db:"opchum" json:"OPCHum"`                   // OPC humidity in percent
	OPCFanRevcount    uint16 `db:"opcfanrevcount" json:"OPCFanRevCount"`   // OPC fan rev count
	OPCLaserStatus    uint16 `db:"opclaserstatus" json:"OPCLaserStatus"`   // OPC laser status

	OPCBin0  uint16 `db:"opcbin_0" json:"OPCBin0"`   // OPC PM bin 0
	OPCBin1  uint16 `db:"opcbin_1" json:"OPCBin1"`   // OPC PM bin 1
	OPCBin2  uint16 `db:"opcbin_2" json:"OPCBin2"`   // OPC PM bin 2
	OPCBin3  uint16 `db:"opcbin_3" json:"OPCBin3"`   // OPC PM bin 3
	OPCBin4  uint16 `db:"opcbin_4" json:"OPCBin4"`   // OPC PM bin 4
	OPCBin5  uint16 `db:"opcbin_5" json:"OPCBin5"`   // OPC PM bin 5
	OPCBin6  uint16 `db:"opcbin_6" json:"OPCBin6"`   // OPC PM bin 6
	OPCBin7  uint16 `db:"opcbin_7" json:"OPCBin7"`   // OPC PM bin 7
	OPCBin8  uint16 `db:"opcbin_8" json:"OPCBin8"`   // OPC PM bin 8
	OPCBin9  uint16 `db:"opcbin_9" json:"OPCBin9"`   // OPC PM bin 9
	OPCBin10 uint16 `db:"opcbin_10" json:"OPCBin10"` // OPC PM bin 10
	OPCBin11 uint16 `db:"opcbin_11" json:"OPCBin11"` // OPC PM bin 11
	OPCBin12 uint16 `db:"opcbin_12" json:"OPCBin12"` // OPC PM bin 12
	OPCBin13 uint16 `db:"opcbin_13" json:"OPCBin13"` // OPC PM bin 13
	OPCBin14 uint16 `db:"opcbin_14" json:"OPCBin14"` // OPC PM bin 14
	OPCBin15 uint16 `db:"opcbin_15" json:"OPCBin15"` // OPC PM bin 15
	OPCBin16 uint16 `db:"opcbin_16" json:"OPCBin16"` // OPC PM bin 16
	OPCBin17 uint16 `db:"opcbin_17" json:"OPCBin17"` // OPC PM bin 17
	OPCBin18 uint16 `db:"opcbin_18" json:"OPCBin18"` // OPC PM bin 18
	OPCBin19 uint16 `db:"opcbin_19" json:"OPCBin19"` // OPC PM bin 19
	OPCBin20 uint16 `db:"opcbin_20" json:"OPCBin20"` // OPC PM bin 20
	OPCBin21 uint16 `db:"opcbin_21" json:"OPCBin21"` // OPC PM bin 21
	OPCBin22 uint16 `db:"opcbin_22" json:"OPCBin22"` // OPC PM bin 22
	OPCBin23 uint16 `db:"opcbin_23" json:"OPCBin23"` // OPC PM bin 23

	OPCSampleValid uint8 `db:"opcsamplevalid" json:"sampleValid"` // OPC Sample valid
}

// Device contains the device information
type Device struct {
	ID           string `db:"id" json:"id"`
	Name         string `db:"name" json:"name"`
	CollectionID string `db:"collection_id" json:"collectionID"`
}

// Cal contains the calibration data for a device.
type Cal struct {
	ID        int64     `db:"id" json:"id"`
	DeviceID  string    `db:"device_id" json:"deviceID"`
	ValidFrom time.Time `db:"valid_from" json:"from"`
	ValidTo   time.Time `db:"valid_to" json:"to"`

	// TODO(borud): fill in fields for calibration data
}

package model

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
)

// DataPoint contains data from air quality sensor.  This type is part
// of the API so this is what the protobuffer gets translated into.
// This way we decouple the protobuffer datatype from the internal
// representation.
//
type DataPoint struct {
	// Board fields
<<<<<<< HEAD
	SysID            int64   `json:"sysID"`            // System id, CPU id or similar
	FirmwareVersion  int64   `json:"firmwareVersion"`  // Firmware version
=======
	SysID            uint64  `json:"sysID"`            // System id, CPU id or similar
	FirmwareVersion  uint64  `json:"firmwareVersion"`  // Firmware version
>>>>>>> 39dfd5ce20b5914f350809975d68b62c89ae7e17
	Uptime           int64   `json:"uptime"`           // Number of milliseconds since boot
	BoardTemp        float32 `json:"boardTemp"`        // Board Temperature, celsius
	BoardRelHumidity float32 `json:"boardRelHumidity"` // Board relative Humidity, percent

	// GPS fields
	GPSTimeStamp float32 `json:"gpsTimestamp"` // GPS Timestamp, seconds since epoch
<<<<<<< HEAD
	Long         float32 `json:"long"`         // Longitude in radians
=======
	Lon          float32 `json:"long"`         // Longitude in radians
>>>>>>> 39dfd5ce20b5914f350809975d68b62c89ae7e17
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
<<<<<<< HEAD
	OPCFanRPM         uint16     `json:"OPCFanRevCount"`  // OPC fan rev count
=======
	OPCHum            uint16     `json:"OPCHum"`          // OPC humidity in percent
	OPCFanRevcount    uint16     `json:"OPCFanRevCount"`  // OPC fan rev count
>>>>>>> 39dfd5ce20b5914f350809975d68b62c89ae7e17
	OPCLaserStatus    uint16     `json:"OPCLaserStatus"`  // OPC laser status
	OPCBins           [24]uint16 `json:"OPCBins"`         // OPC PM bins 0-23
	OPCSampleValid    uint8      `json:"sampleValid"`     // OPC Sample valid
}

// DataPointFromProtobuf takes a Sample protobuffer and returns a
// DataPoint
func DataPointFromProtobuf(s *aqpb.Sample) *DataPoint {
	return &DataPoint{
		// Board fields
		SysID:            s.Sysid,
		FirmwareVersion:  s.FirmwareVersion,
		Uptime:           s.Uptime,
		BoardTemp:        s.BoardTemp,
		BoardRelHumidity: s.BoardRelHumidity,

		// GPS fields
		GPSTimeStamp: s.GpsTimestamp,
		Lat:          s.Lat,
<<<<<<< HEAD
		Long:         s.Long,
=======
		Lon:          s.Lon,
>>>>>>> 39dfd5ce20b5914f350809975d68b62c89ae7e17
		Alt:          s.Alt,

		// AFE3 fields
		Sensor1Work: s.Sensor_1Work,
		Sensor1Aux:  s.Sensor_1Aux,
		Sensor2Work: s.Sensor_2Work,
		Sensor2Aux:  s.Sensor_2Aux,
		Sensor3Work: s.Sensor_3Work,
		Sensor32Aux: s.Sensor_3Aux,
		AFE3Temp:    s.Afe3Temp,

		// OPC-N3 fields
		OPCPMA:            s.OpcPmA,
		OPCPMB:            s.OpcPmB,
		OPCPMC:            s.OpcPmC,
		OPCSamplePeriod:   uint16(s.OpcSamplePeriod),
		OPCSampleFlowRate: uint16(s.OpcSampleFlowRate),
		OPCTemp:           uint16(s.OpcTemp),
<<<<<<< HEAD
		OPCFanRPM:         uint16(s.OpcFanRpm),
=======
		OPCHum:            uint16(s.OpcHum),
		OPCFanRevcount:    uint16(s.OpcFanRevcount),
>>>>>>> 39dfd5ce20b5914f350809975d68b62c89ae7e17
		OPCLaserStatus:    uint16(s.OpcLaserStatus),
		OPCSampleValid:    uint8(s.OpcSampleValid),

		OPCBins: [24]uint16{
			uint16(s.OpcBin_0), uint16(s.OpcBin_1), uint16(s.OpcBin_2), uint16(s.OpcBin_3), uint16(s.OpcBin_4),
			uint16(s.OpcBin_5), uint16(s.OpcBin_6), uint16(s.OpcBin_7), uint16(s.OpcBin_8), uint16(s.OpcBin_9),
			uint16(s.OpcBin_10), uint16(s.OpcBin_11), uint16(s.OpcBin_12), uint16(s.OpcBin_13), uint16(s.OpcBin_14),
			uint16(s.OpcBin_15), uint16(s.OpcBin_16), uint16(s.OpcBin_17), uint16(s.OpcBin_18), uint16(s.OpcBin_19),
			uint16(s.OpcBin_20), uint16(s.OpcBin_21), uint16(s.OpcBin_22), uint16(s.OpcBin_23),
		},
	}
}

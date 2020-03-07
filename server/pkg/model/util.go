package model

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
	"google.golang.org/protobuf/proto"
)

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
		Status:           s.Status,

		// GPS fields
		GPSTimeStamp: s.GpsTimestamp,
		Lat:          s.Lat,
		Lon:          s.Lon,
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
		OPCHum:            uint16(s.OpcHum),
		OPCFanRevcount:    uint16(s.OpcFanRevcount),
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

// ProtobufFromData unmarshals a protobuffer from a byte slice
func ProtobufFromData(buf []byte) (*aqpb.Sample, error) {
	pb := aqpb.Sample{}
	return &pb, proto.Unmarshal(buf, &pb)
}

// DataFromProtobuf marshals a aqpb.Sample into a byte slice
func DataFromProtobuf(pb *aqpb.Sample) ([]byte, error) {
	return proto.Marshal(pb)
}

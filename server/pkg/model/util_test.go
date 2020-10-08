package model

import (
	"testing"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/assert"
)

var testPB = &aqpb.Sample{
	Sysid:             1,
	FirmwareVersion:   2,
	Uptime:            3,
	BoardTemp:         4.0,
	BoardRelHumidity:  5.0,
	Status:            6,
	GpsTimestamp:      7.0,
	Lat:               8.0,
	Lon:               9.0,
	Alt:               10.0,
	Sensor_1Work:      11,
	Sensor_1Aux:       12,
	Sensor_2Work:      13,
	Sensor_2Aux:       14,
	Sensor_3Work:      15,
	Sensor_3Aux:       16,
	Afe3TempRaw:       17,
	OpcPmA:            18,
	OpcPmB:            19,
	OpcPmC:            20,
	Pm1:               18.1,
	Pm10:              19.1,
	Pm25:              20.1,
	OpcSamplePeriod:   21,
	OpcSampleFlowRate: 22,
	OpcTemp:           23,
	OpcHum:            24,
	OpcFanRevcount:    25,
	OpcLaserStatus:    26,
	OpcSampleValid:    27,
	OpcBin_0:          28,
	OpcBin_2:          29,
	OpcBin_3:          30,
	OpcBin_4:          31,
	OpcBin_5:          32,
	OpcBin_6:          33,
	OpcBin_7:          34,
	OpcBin_8:          35,
	OpcBin_9:          36,
	OpcBin_10:         37,
	OpcBin_11:         38,
	OpcBin_12:         39,
	OpcBin_13:         40,
	OpcBin_14:         41,
	OpcBin_15:         42,
	OpcBin_16:         43,
	OpcBin_17:         44,
	OpcBin_18:         45,
	OpcBin_19:         46,
	OpcBin_20:         47,
	OpcBin_21:         48,
	OpcBin_22:         49,
	OpcBin_23:         50,
}
var testPBLen = 176

func TestSimple(t *testing.T) {
	// Create a byte array from protobuffer instance
	buf, err := DataFromProtobuf(testPB)
	assert.Nil(t, err, "Failed to serialize PB")
	assert.Equal(t, testPBLen, len(buf), "PB is the wrong length")

	// Get a protobuf instance from the buffer
	pb, err := ProtobufFromData(buf)
	assert.Nil(t, err, "Failed to deserialize PB")
	assert.True(t, proto.Equal(testPB, pb), "Protobuffers not equal")

	// Create a Message from the protobuffer
	dp := MessageFromProtobuf(pb)
	assert.NotNil(t, dp)

	assert.Equal(t, testPB.Sysid, dp.SysID)
	assert.Equal(t, testPB.FirmwareVersion, dp.FirmwareVersion)
	assert.Equal(t, testPB.Uptime, dp.Uptime)
	assert.Equal(t, testPB.BoardTemp, dp.BoardTemp)
	assert.Equal(t, testPB.BoardRelHumidity, dp.BoardRelHumidity)
	assert.Equal(t, testPB.Status, dp.Status)

	assert.Equal(t, testPB.GpsTimestamp, dp.GPSTimeStamp)
	assert.Equal(t, testPB.Lat, dp.Lat)
	assert.Equal(t, testPB.Lon, dp.Lon)
	assert.Equal(t, testPB.Alt, dp.Alt)

	assert.Equal(t, testPB.Sensor_1Work, dp.Sensor1Work)
	assert.Equal(t, testPB.Sensor_1Aux, dp.Sensor1Aux)
	assert.Equal(t, testPB.Sensor_2Work, dp.Sensor2Work)
	assert.Equal(t, testPB.Sensor_2Aux, dp.Sensor2Aux)
	assert.Equal(t, testPB.Sensor_3Work, dp.Sensor3Work)
	assert.Equal(t, testPB.Sensor_3Aux, dp.Sensor3Aux)

	assert.Equal(t, testPB.Afe3TempRaw, dp.AFE3TempRaw)

	assert.Equal(t, testPB.OpcPmA, dp.OPCPMA)
	assert.Equal(t, testPB.OpcPmB, dp.OPCPMB)
	assert.Equal(t, testPB.OpcPmC, dp.OPCPMC)

	assert.Equal(t, uint16(testPB.OpcSamplePeriod), dp.OPCSamplePeriod)
	assert.Equal(t, uint16(testPB.OpcSampleFlowRate), dp.OPCSampleFlowRate)
	assert.Equal(t, uint16(testPB.OpcTemp), dp.OPCTemp)
	assert.Equal(t, uint16(testPB.OpcFanRevcount), dp.OPCFanRevcount)
	assert.Equal(t, uint16(testPB.OpcLaserStatus), dp.OPCLaserStatus)
	assert.Equal(t, uint8(testPB.OpcSampleValid), dp.OPCSampleValid)

	assert.Equal(t, uint16(testPB.OpcBin_0), dp.OPCBin0)
	assert.Equal(t, uint16(testPB.OpcBin_1), dp.OPCBin1)
	assert.Equal(t, uint16(testPB.OpcBin_2), dp.OPCBin2)
	assert.Equal(t, uint16(testPB.OpcBin_3), dp.OPCBin3)
	assert.Equal(t, uint16(testPB.OpcBin_4), dp.OPCBin4)
	assert.Equal(t, uint16(testPB.OpcBin_5), dp.OPCBin5)
	assert.Equal(t, uint16(testPB.OpcBin_6), dp.OPCBin6)
	assert.Equal(t, uint16(testPB.OpcBin_7), dp.OPCBin7)
	assert.Equal(t, uint16(testPB.OpcBin_8), dp.OPCBin8)
	assert.Equal(t, uint16(testPB.OpcBin_9), dp.OPCBin9)
	assert.Equal(t, uint16(testPB.OpcBin_10), dp.OPCBin10)
	assert.Equal(t, uint16(testPB.OpcBin_11), dp.OPCBin11)
	assert.Equal(t, uint16(testPB.OpcBin_12), dp.OPCBin12)
	assert.Equal(t, uint16(testPB.OpcBin_13), dp.OPCBin13)
	assert.Equal(t, uint16(testPB.OpcBin_14), dp.OPCBin14)
	assert.Equal(t, uint16(testPB.OpcBin_15), dp.OPCBin15)
	assert.Equal(t, uint16(testPB.OpcBin_16), dp.OPCBin16)
	assert.Equal(t, uint16(testPB.OpcBin_17), dp.OPCBin17)
	assert.Equal(t, uint16(testPB.OpcBin_18), dp.OPCBin18)
	assert.Equal(t, uint16(testPB.OpcBin_19), dp.OPCBin19)
	assert.Equal(t, uint16(testPB.OpcBin_20), dp.OPCBin20)
	assert.Equal(t, uint16(testPB.OpcBin_21), dp.OPCBin21)
	assert.Equal(t, uint16(testPB.OpcBin_22), dp.OPCBin22)
	assert.Equal(t, uint16(testPB.OpcBin_23), dp.OPCBin23)

}

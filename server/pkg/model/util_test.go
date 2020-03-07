package model

import (
	"testing"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/proto"
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
	Afe3Temp:          17,
	OpcPmA:            18,
	OpcPmB:            19,
	OpcPmC:            20,
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
var testPBLen = 158

func TestSimple(t *testing.T) {
	// Create a byte array from protobuffer instance
	buf, err := DataFromProtobuf(testPB)
	assert.Nil(t, err, "Failed to serialize PB")
	assert.Equal(t, testPBLen, len(buf), "PB is the wrong length")

	// Get a protobuf instance from the buffer
	pb, err := ProtobufFromData(buf)
	assert.Nil(t, err, "Failed to deserialize PB")
	assert.True(t, proto.Equal(testPB, pb), "Protobuffers not equal")

	// Create a datapoint from the protobuffer
	dp := DataPointFromProtobuf(pb)
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
	assert.Equal(t, testPB.Sensor_3Aux, dp.Sensor32Aux)

	assert.Equal(t, testPB.Afe3Temp, dp.AFE3Temp)

	assert.Equal(t, testPB.OpcPmA, dp.OPCPMA)
	assert.Equal(t, testPB.OpcPmB, dp.OPCPMB)
	assert.Equal(t, testPB.OpcPmC, dp.OPCPMC)

	assert.Equal(t, uint16(testPB.OpcSamplePeriod), dp.OPCSamplePeriod)
	assert.Equal(t, uint16(testPB.OpcSampleFlowRate), dp.OPCSampleFlowRate)
	assert.Equal(t, uint16(testPB.OpcTemp), dp.OPCTemp)
	assert.Equal(t, uint16(testPB.OpcFanRevcount), dp.OPCFanRevcount)
	assert.Equal(t, uint16(testPB.OpcLaserStatus), dp.OPCLaserStatus)
	assert.Equal(t, uint8(testPB.OpcSampleValid), dp.OPCSampleValid)

	assert.Equal(t, uint16(testPB.OpcBin_0), dp.OPCBins[0])
	assert.Equal(t, uint16(testPB.OpcBin_1), dp.OPCBins[1])
	assert.Equal(t, uint16(testPB.OpcBin_2), dp.OPCBins[2])
	assert.Equal(t, uint16(testPB.OpcBin_3), dp.OPCBins[3])
	assert.Equal(t, uint16(testPB.OpcBin_4), dp.OPCBins[4])
	assert.Equal(t, uint16(testPB.OpcBin_5), dp.OPCBins[5])
	assert.Equal(t, uint16(testPB.OpcBin_6), dp.OPCBins[6])
	assert.Equal(t, uint16(testPB.OpcBin_7), dp.OPCBins[7])
	assert.Equal(t, uint16(testPB.OpcBin_8), dp.OPCBins[8])
	assert.Equal(t, uint16(testPB.OpcBin_9), dp.OPCBins[9])
	assert.Equal(t, uint16(testPB.OpcBin_10), dp.OPCBins[10])
	assert.Equal(t, uint16(testPB.OpcBin_11), dp.OPCBins[11])
	assert.Equal(t, uint16(testPB.OpcBin_12), dp.OPCBins[12])
	assert.Equal(t, uint16(testPB.OpcBin_13), dp.OPCBins[13])
	assert.Equal(t, uint16(testPB.OpcBin_14), dp.OPCBins[14])
	assert.Equal(t, uint16(testPB.OpcBin_15), dp.OPCBins[15])
	assert.Equal(t, uint16(testPB.OpcBin_16), dp.OPCBins[16])
	assert.Equal(t, uint16(testPB.OpcBin_17), dp.OPCBins[17])
	assert.Equal(t, uint16(testPB.OpcBin_18), dp.OPCBins[18])
	assert.Equal(t, uint16(testPB.OpcBin_19), dp.OPCBins[19])
	assert.Equal(t, uint16(testPB.OpcBin_20), dp.OPCBins[20])
	assert.Equal(t, uint16(testPB.OpcBin_21), dp.OPCBins[21])
	assert.Equal(t, uint16(testPB.OpcBin_22), dp.OPCBins[22])
	assert.Equal(t, uint16(testPB.OpcBin_23), dp.OPCBins[23])

}

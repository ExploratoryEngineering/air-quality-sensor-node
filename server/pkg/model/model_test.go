package model

import (
	"testing"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/aqpb"
	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/proto"
)

func TestSimple(t *testing.T) {
	s := &aqpb.Sample{
		OpcBin_0:  1,
		OpcBin_4:  10,
		OpcBin_8:  20,
		OpcBin_12: 30,
		OpcBin_23: 23,
	}

	buf, err := proto.Marshal(s)
	assert.Nil(t, err)

	sample := aqpb.Sample{}
	err = proto.Unmarshal(buf, &sample)
	assert.Nil(t, err)

	dp := DataPointFromProtobuf(&sample)
	assert.NotNil(t, dp)

	dp = DataPointFromProtobuf(&sample)
	assert.NotNil(t, dp)

	assert.Equal(t, uint16(s.OpcBin_0), dp.OPCBins[0])
	assert.Equal(t, uint16(s.OpcBin_4), dp.OPCBins[4])
	assert.Equal(t, uint16(s.OpcBin_8), dp.OPCBins[8])
	assert.Equal(t, uint16(s.OpcBin_12), dp.OPCBins[12])
	assert.Equal(t, uint16(s.OpcBin_23), dp.OPCBins[23])
}

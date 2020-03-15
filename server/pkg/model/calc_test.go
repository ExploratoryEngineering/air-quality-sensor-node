package model

import (
	"math"
	"testing"

	"github.com/stretchr/testify/assert"
)

// TODO(borud): we need sensible testdata from at least a couple of
// sensors so we can ensure the calculation is correct and that we get
// some sensible values.  Part of the testdata should come from
// flooding the AFE3 sensors with calibration mixtures.
//
var calTest = &Cal{
	// value from deviceID '17dh0cf43jg6n4'. Measured by Thomas Langås
	// in the lab under relatively stable conditions.  Can use 0.32 as
	// default VT20Offset if we lack measured offset value.
	Vt20Offset: 0.3195,

	Sensor1WorkingElectrodeElectronicOffset: 312,
	Sensor1WorkingElectrodeSensorZero:       -5,
	Sensor1AuxElectrodeElectronicOffset:     316,
	Sensor1AuxElectrodeSensorZero:           -5,
	Sensor1PCBGain:                          -0.73,
	Sensor1WorkingElectrodeSensitivity:      0.203,

	Sensor2WorkingElectrodeElectronicOffset: 411,
	Sensor2WorkingElectrodeSensorZero:       -4,
	Sensor2AuxElectrodeElectronicOffset:     411,
	Sensor2AuxElectrodeSensorZero:           -3,
	Sensor2PCBGain:                          -0.73,
	Sensor2WorkingElectrodeSensitivity:      0.363,

	Sensor3WorkingElectrodeElectronicOffset: 271,
	Sensor3WorkingElectrodeSensorZero:       19,
	Sensor3AuxElectrodeElectronicOffset:     256,
	Sensor3AuxElectrodeSensorZero:           23,
	Sensor3PCBGain:                          0.8,
	Sensor3WorkingElectrodeSensitivity:      0.408,
}
var messageTest = &Message{
	Sensor1Work: 519656,
	Sensor1Aux:  522293,

	Sensor2Work: 697336,
	Sensor2Aux:  682847,

	Sensor3Work: 443429,
	Sensor3Aux:  431900,

	AFE3Temp: 540375,
}

func TestCalculateSensorValues(t *testing.T) {
	CalculateSensorValues(messageTest, calTest)
}

func TestAFE3LUTs(t *testing.T) {
	// Ensure the lookup tables are correct
	assert.Equal(t, afe3Luts["CO-A4"].LUT, []float64{1.0, 1.0, 1.0, 1.0, 1.0, -1.0, -0.76, -0.76 - 0.76})
	assert.Equal(t, afe3Luts["CO2-B4"].LUT, []float64{-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -3.8 - 3.8 - 3.8})
	assert.Equal(t, afe3Luts["NO-A4"].LUT, []float64{1.48, 1.48, 1.48, 1.48, 1.48, 2.02, 1.72, 1.72, 1.72})
	assert.Equal(t, afe3Luts["NO-B4"].LUT, []float64{1.04, 1.04, 1.04, 1.04, 1.04, 1.82, 2.0, 2.0, 2.0})
	assert.Equal(t, afe3Luts["NO2-A4"].LUT, []float64{1.09, 1.09, 1.09, 1.09, 1.09, 1.35, 3.0, 3.0, 3.0})
	assert.Equal(t, afe3Luts["NO2-B4"].LUT, []float64{0.76, 0.76, 0.76, 0.76, 0.76, 0.68, 0.23, 0.23, 0.23})
	assert.Equal(t, afe3Luts["SO2-A4"].LUT, []float64{1.15, 1.15, 1.15, 1.15, 1.15, 1.82, 3.93, 3.93, 3.93})
	assert.Equal(t, afe3Luts["SO2-B4"].LUT, []float64{0.96, 0.96, 0.96, 0.96, 0.96, 1.34, 1.10, 1.10, 1.10})
	assert.Equal(t, afe3Luts["O3-A4"].LUT, []float64{0.75, 0.75, 0.75, 0.75, 1.28, 1.28, 1.28, 1.28})
	assert.Equal(t, afe3Luts["O3-B4"].LUT, []float64{0.77, 0.77, 0.77, 0.77, 1.56, 1.56, 1.56, 2.85})
}

func TestSplineFuncFromName(t *testing.T) {
	// Should be good enough
	const tolerance = 0.000000001

	compare := func(x, y float64) bool {
		diff := math.Abs(x - y)
		mean := math.Abs(x+y) / 2.0
		return (diff / mean) < tolerance
	}

	//	perform a simple test to check that the spline matching is
	//	within reasonable limits for at least the spline points
	for k, v := range afe3Luts {
		f := correctionFuncFromName(k)
		assert.NotNil(t, f)

		for i := 0; i < len(v.LUT); i++ {
			assert.True(t, compare(v.LUT[i], f(afe3LutTemperatures[i])))
		}
	}
}

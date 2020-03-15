package model

import (
	"log"

	"github.com/sgreben/piecewiselinear"
)

const (
	voltageAt20DegreesCentigrade = 0.297
	genericVt20Offset            = 0.32
)

type sensorLut struct {
	LUT []float64
}

var (
	afe3ScalingFactor = float64(0.0000005960464478) // named "lsb" in datasheet

	// Temperatures used in the LUT
	afe3LutTemperatures = []float64{-30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0}

	// Lookup tables according to Appendex 1 of Alphasense Application Note AAN 803
	afe3Luts = map[string]sensorLut{
		"CO-A4":  sensorLut{LUT: []float64{1.0, 1.0, 1.0, 1.0, 1.0, -1.0, -0.76, -0.76 - 0.76}},
		"CO2-B4": sensorLut{LUT: []float64{-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -3.8 - 3.8 - 3.8}},
		"NO-A4":  sensorLut{LUT: []float64{1.48, 1.48, 1.48, 1.48, 1.48, 2.02, 1.72, 1.72, 1.72}},
		"NO-B4":  sensorLut{LUT: []float64{1.04, 1.04, 1.04, 1.04, 1.04, 1.82, 2.0, 2.0, 2.0}},
		"NO2-A4": sensorLut{LUT: []float64{1.09, 1.09, 1.09, 1.09, 1.09, 1.35, 3.0, 3.0, 3.0}},
		"NO2-B4": sensorLut{LUT: []float64{0.76, 0.76, 0.76, 0.76, 0.76, 0.68, 0.23, 0.23, 0.23}},
		"SO2-A4": sensorLut{LUT: []float64{1.15, 1.15, 1.15, 1.15, 1.15, 1.82, 3.93, 3.93, 3.93}},
		"SO2-B4": sensorLut{LUT: []float64{0.96, 0.96, 0.96, 0.96, 0.96, 1.34, 1.10, 1.10, 1.10}},
		"O3-A4":  sensorLut{LUT: []float64{0.75, 0.75, 0.75, 0.75, 1.28, 1.28, 1.28, 1.28 /*, no value */}},
		"O3-B4":  sensorLut{LUT: []float64{0.77, 0.77, 0.77, 0.77, 1.56, 1.56, 1.56, 2.85 /*, no value */}},
	}
)

// CalculateSensorValues calculates sensor values using measured data
// and calibration data
func CalculateSensorValues(m *Message, cal *Cal) {

	// Calculate the temperature.
	// TODO(borud): have @tlan and @hansj double-check this
	afe3Temp := ((float64(m.AFE3Temp) * afe3ScalingFactor) - cal.Vt20Offset + 0.02) * 1000.0

	// TODO(borud): There is a lookup table in Alphasense Application
	// Note "AAN 803"
	//
	var sensor1TempCorrectionFactor = 1.35
	var sensor2TempCorrectionFactor = 1.28
	var sensor3TempCorrectionFactor = 2.02

	var S1 float64
	var S2 float64
	var S3 float64

	// Sensor 1 - NO2 sensor
	{
		wmV := voltage(m.Sensor1Work, cal.Sensor1WorkingElectrodeElectronicOffset, cal.Sensor1WorkingElectrodeSensorZero)
		amV := voltage(m.Sensor1Aux, cal.Sensor1AuxElectrodeElectronicOffset, cal.Sensor1AuxElectrodeSensorZero) * sensor1TempCorrectionFactor
		S1 = (wmV - amV) / cal.Sensor1WorkingElectrodeSensitivity
	}

	// Sensor 2 - O3 + NO2 sensor
	{
		wmV := voltage(m.Sensor2Work, cal.Sensor2WorkingElectrodeElectronicOffset, cal.Sensor2WorkingElectrodeSensorZero)
		amV := voltage(m.Sensor2Aux, cal.Sensor2AuxElectrodeElectronicOffset, cal.Sensor2AuxElectrodeSensorZero) * sensor2TempCorrectionFactor
		S2 = (wmV - amV) / cal.Sensor2WorkingElectrodeSensitivity
	}

	// Sensor 3 - NO sensor
	{
		wmV := voltage(m.Sensor3Work, cal.Sensor3WorkingElectrodeElectronicOffset, cal.Sensor3WorkingElectrodeSensorZero)
		amV := voltage(m.Sensor3Aux, cal.Sensor3AuxElectrodeElectronicOffset, cal.Sensor3AuxElectrodeSensorZero) * sensor3TempCorrectionFactor
		S3 = (wmV - amV) / cal.Sensor1WorkingElectrodeSensitivity
	}

	log.Printf("AFE3Temp  (C): %f", afe3Temp)
	log.Printf("NO2     (ppb): %f", S1)
	log.Printf("NO2+O3  (ppb): %f", S2)
	log.Printf("NO      (ppb): %f", S3)
}

func voltage(w uint32, offset int32, zero int32) float64 {
	return (float64(w) * afe3ScalingFactor * 1000) - float64(offset+zero)
}

func correctionFuncFromName(name string) func(float64) float64 {
	lut, ok := afe3Luts[name]
	if !ok {
		return nil
	}

	f := piecewiselinear.Function{Y: lut.LUT}
	f.X = afe3LutTemperatures[:len(lut.LUT)]

	return func(t float64) float64 {
		return f.At(t)
	}
}

package model

import "testing"

var calTest = &Cal{
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

	AFE3Temp: 541633,
}

func TestCalculateSensorValues(t *testing.T) {
	CalculateSensorValues(messageTest, calTest)
}

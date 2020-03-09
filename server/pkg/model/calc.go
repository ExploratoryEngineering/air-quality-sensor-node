package model

import "log"

const voltageAt20DegreesCentigrade = 0.297

var afe3ScalingFactor float64 = float64(0.0000005960464478) // named "lsb" in datasheet

// CalculateSensorValues calculates sensor values using measured data
// and calibration data
func CalculateSensorValues(m *Message, cal *Cal) {

	// Calculate the temperature
	afe3Temp := ((float64(m.AFE3Temp) * afe3ScalingFactor) - 0.297) * 1000.0
	log.Printf("AFE3Temp : %f", afe3Temp)

	// TODO(borud): Currently we do not calculate the temperature so
	// this factor is just pinned at 1.0 right now.  There is a lookup
	// table in Alphasense Application Note "AAN 803"
	//
	var sensor1TempCorrectionFactor float64 = 1.35
	var sensor2TempCorrectionFactor float64 = 1.28
	var sensor3TempCorrectionFactor float64 = 2.02

	sensor1WorkingElectrodeVoltage := ((float64(m.Sensor1Work) * afe3ScalingFactor) * 1000) - float64(cal.Sensor1WorkingElectrodeElectronicOffset+cal.Sensor1WorkingElectrodeSensorZero)
	sensor1AuxElectrodeVoltage := ((float64(m.Sensor1Aux) * afe3ScalingFactor) * 1000) - float64(cal.Sensor1AuxElectrodeElectronicOffset+cal.Sensor1AuxElectrodeSensorZero)
	sensor1AuxElectrodeVoltage = sensor1AuxElectrodeVoltage * sensor1TempCorrectionFactor
	correctedSensor1Voltage := sensor1WorkingElectrodeVoltage - sensor1AuxElectrodeVoltage
	sensorValue1 := correctedSensor1Voltage / cal.Sensor1WorkingElectrodeSensitivity

	log.Printf("")
	log.Printf("Sensor1Work                    : %d", m.Sensor1Work)
	log.Printf("Sensor1Aux                     : %d", m.Sensor1Aux)
	log.Printf("sensor1WorkingElectrodeVoltage : %f", sensor1WorkingElectrodeVoltage)
	log.Printf("sensor1AuxElectrodeVoltage     : %f", sensor1AuxElectrodeVoltage)
	log.Printf("correctedSensor1Voltage        : %f (mV)", correctedSensor1Voltage)
	log.Printf("sensorValue1                   : %f (ppb)", sensorValue1)

	sensor2WorkingElectrodeVoltage := ((float64(m.Sensor2Work) * afe3ScalingFactor) * 1000) - float64(cal.Sensor2WorkingElectrodeElectronicOffset+cal.Sensor2WorkingElectrodeSensorZero)
	sensor2AuxElectrodeVoltage := ((float64(m.Sensor2Aux) * afe3ScalingFactor) * 1000) - float64(cal.Sensor2AuxElectrodeElectronicOffset+cal.Sensor2AuxElectrodeSensorZero)
	sensor2AuxElectrodeVoltage = sensor2AuxElectrodeVoltage * sensor2TempCorrectionFactor
	correctedSensor2Voltage := sensor2WorkingElectrodeVoltage - sensor2AuxElectrodeVoltage
	sensorValue2 := correctedSensor2Voltage / cal.Sensor2WorkingElectrodeSensitivity

	log.Printf("")
	log.Printf("Sensor2Work                    : %d", m.Sensor2Work)
	log.Printf("Sensor2Aux                     : %d", m.Sensor2Aux)
	log.Printf("sensor2WorkingElectrodeVoltage : %f", sensor2WorkingElectrodeVoltage)
	log.Printf("sensor2AuxElectrodeVoltage     : %f", sensor2AuxElectrodeVoltage)
	log.Printf("correctedSensor2Voltage        : %f (mV)", correctedSensor2Voltage)
	log.Printf("sensorValue2                   : %f (ppb)", sensorValue2)
	log.Printf("sensorValue2 (corr)            : %f (ppb)", sensorValue2-sensorValue1)

	sensor3WorkingElectrodeVoltage := ((float64(m.Sensor3Work) * afe3ScalingFactor) * 1000) - float64(cal.Sensor3WorkingElectrodeElectronicOffset+cal.Sensor3WorkingElectrodeSensorZero)
	sensor3AuxElectrodeVoltage := ((float64(m.Sensor3Aux) * afe3ScalingFactor) * 1000) - float64(cal.Sensor3AuxElectrodeElectronicOffset+cal.Sensor3AuxElectrodeSensorZero)
	sensor3AuxElectrodeVoltage = sensor3AuxElectrodeVoltage * sensor3TempCorrectionFactor
	correctedSensor3Voltage := sensor3WorkingElectrodeVoltage - sensor3AuxElectrodeVoltage
	sensorValue3 := correctedSensor3Voltage / cal.Sensor3WorkingElectrodeSensitivity

	log.Printf("")
	log.Printf("Sensor3Work                    : %d", m.Sensor3Work)
	log.Printf("Sensor3Aux                     : %d", m.Sensor3Aux)
	log.Printf("sensor3WorkingElectrodeVoltage : %f", sensor3WorkingElectrodeVoltage)
	log.Printf("sensor3AuxElectrodeVoltage     : %f", sensor3AuxElectrodeVoltage)
	log.Printf("correctedSensor3Voltage        : %f (mV)", correctedSensor3Voltage)
	log.Printf("sensorValue3                   : %f (ppb)", sensorValue3)
}

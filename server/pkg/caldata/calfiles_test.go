package caldata

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestReadFromDir(t *testing.T) {
	cals, err := readCalsFromDir("../../calibration-data", "*.json")
	assert.Nil(t, err)
	assert.NotNil(t, cals)
	assert.NotEqual(t, 0, len(cals))
}

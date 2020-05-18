package api

import (
	"io/ioutil"
	"log"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
)

// Just make sure that the server starts and terminates.
func TestAPISimple(t *testing.T) {
	tempLogDir, err := ioutil.TempDir("", "testlog")
	assert.Nil(t, err)
	defer os.RemoveAll(tempLogDir)

	log.Print(tempLogDir)

	s := New(&ServerConfig{
		ListenAddr:   ":0",
		StaticDir:    "../../static",
		TemplateDir:  "../../templates",
		AccessLogDir: tempLogDir,
	})
	assert.NotNil(t, s)
	s.Start()
	s.Shutdown()
}

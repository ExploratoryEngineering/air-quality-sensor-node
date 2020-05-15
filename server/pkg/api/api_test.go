package api

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

// Just make sure that the server starts and terminates.
func TestAPISimple(t *testing.T) {
	s := New(&ServerConfig{
		ListenAddr:  ":0",
		StaticDir:   "../../static",
		TemplateDir: "../../templates",
	})
	assert.NotNil(t, s)
	s.Start()
	s.Shutdown()
}

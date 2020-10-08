package api

import (
	"context"
	"log"
	"net/http"
	"os"
	"path"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
	"github.com/gorilla/handlers"
	"github.com/gorilla/mux"
)

// Server represents the webserver state
type Server struct {
	db             store.Store
	broker         *stream.Broker
	circularBuffer *circular.Buffer
	listenAddr     string
	readTimeout    time.Duration
	writeTimeout   time.Duration
	httpServer     http.Server
	accessLogDir   string
}

// ServerConfig represents the webserver configuration
type ServerConfig struct {
	DB             store.Store
	Broker         *stream.Broker
	CircularBuffer *circular.Buffer
	ListenAddr     string
	AccessLogDir   string
}

const (
	defaultReadTimeout     = (15 * time.Second)
	defaultWriteTimeout    = (30 * time.Second)
	defaultShutdownTimeout = (10 * time.Second)
	accessLogFileMode      = 0644
)

// New creates a new webserver instance
func New(config *ServerConfig) *Server {
	return &Server{
		db:             config.DB,
		broker:         config.Broker,
		circularBuffer: config.CircularBuffer,
		listenAddr:     config.ListenAddr,
		readTimeout:    defaultReadTimeout,
		writeTimeout:   defaultWriteTimeout,
		accessLogDir:   config.AccessLogDir,
	}
}

// Start starts the webserver.  Does not block.
func (s *Server) Start() {
	// Create router
	m := mux.NewRouter().StrictSlash(true)
	m.HandleFunc("/stream", s.streamHandler).Methods("GET")

	// Set up access logging
	if _, err := os.Stat(s.accessLogDir); os.IsNotExist(err) {
		log.Printf("Creating access log directory: %s", s.accessLogDir)
		err := os.MkdirAll(s.accessLogDir, os.ModePerm)
		if err != nil {
			log.Fatalf("Unable to create directory for access log '%s': %v", s.accessLogDir, err)
		}
	}
	accessLogFileName := path.Join(s.accessLogDir, time.Now().Format("2006-01-02-access-log"))
	accessLogFile, err := os.OpenFile(accessLogFileName, os.O_CREATE|os.O_APPEND|os.O_WRONLY, accessLogFileMode)
	if err != nil {
		log.Fatalf("Unable to create access log: %v", err)
	}

	// Set up webserver
	server := &http.Server{
		Handler:      handlers.ProxyHeaders(handlers.CombinedLoggingHandler(accessLogFile, m)),
		Addr:         s.listenAddr,
		WriteTimeout: s.readTimeout,
		ReadTimeout:  s.writeTimeout,
	}

	log.Printf("Webserver listening to '%s'", s.listenAddr)
	go func() {
		log.Printf("Webserver terminated: '%v'", server.ListenAndServe())
	}()
}

// Shutdown shuts down the webserver
func (s *Server) Shutdown() {
	ctx, cancel := context.WithTimeout(context.Background(), defaultShutdownTimeout)
	defer cancel()

	err := s.httpServer.Shutdown(ctx)
	if err != nil {
		log.Printf("Webserver shutdown error: %v", err)
	}
}

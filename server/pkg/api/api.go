package api

import (
	"context"
	"html/template"
	"log"
	"net/http"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/circular"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline/stream"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/store"
	"github.com/gorilla/mux"
)

// Server represents the webserver state
type Server struct {
	db             store.Store
	broker         *stream.Broker
	circularBuffer *circular.Buffer
	listenAddr     string
	staticDir      string
	templateDir    string
	templates      *template.Template
	readTimeout    time.Duration
	writeTimeout   time.Duration
	httpServer     http.Server
}

// ServerConfig represents the webserver configuration
type ServerConfig struct {
	DB             store.Store
	Broker         *stream.Broker
	CircularBuffer *circular.Buffer
	ListenAddr     string
	StaticDir      string
	TemplateDir    string
}

const (
	defaultReadTimeout     = (15 * time.Second)
	defaultWriteTimeout    = (30 * time.Second)
	defaultShutdownTimeout = (10 * time.Second)
)

// New creates a new webserver instance
func New(config *ServerConfig) *Server {
	// Yes we could have used template.Must but the output is butt ugly.
	t, err := template.ParseGlob(config.TemplateDir + "/*.html")
	if err != nil {
		log.Fatalf("Error reading templates: %v", err)
	}
	return &Server{
		db:             config.DB,
		broker:         config.Broker,
		circularBuffer: config.CircularBuffer,
		listenAddr:     config.ListenAddr,
		staticDir:      config.StaticDir,
		templateDir:    config.TemplateDir,
		readTimeout:    defaultReadTimeout,
		writeTimeout:   defaultWriteTimeout,
		templates:      t,
	}
}

// Start starts the webserver.  Does not block.
func (s *Server) Start() {
	m := mux.NewRouter().StrictSlash(true)
	m.HandleFunc("/", s.mainHandler).Methods("GET")
	m.HandleFunc("/stream", s.streamHandler).Methods("GET")
	m.HandleFunc("/data", s.lastDataHandler).Methods("GET")

	server := &http.Server{
		Handler:      m,
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

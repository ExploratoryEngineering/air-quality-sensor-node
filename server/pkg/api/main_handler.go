package api

import (
	"log"
	"net/http"
)

func (s *Server) mainHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html")

	clients := s.broker.ListClients()

	type Data struct {
		Clients []string
	}

	err := s.templates.ExecuteTemplate(w, "index.html", &Data{Clients: clients})
	if err != nil {
		log.Printf("Error executing template: %v", err)
	}
}

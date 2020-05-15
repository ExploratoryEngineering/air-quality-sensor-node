package api

import (
	"log"
	"net/http"
)

func (s *Server) mainHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html")
	err := s.templates.ExecuteTemplate(w, "index.html", nil)
	if err != nil {
		log.Printf("Error executing template: %v", err)
	}
}

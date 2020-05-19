package api

import (
	"log"
	"net/http"
)

func (s *Server) lastDataHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html")

	data := s.circularBuffer.GetContents()

	err := s.templates.ExecuteTemplate(w, "last.html", data)
	if err != nil {
		log.Printf("Error executing template: %v", err)
	}
}

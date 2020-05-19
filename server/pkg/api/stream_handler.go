package api

import (
	"log"
	"net/http"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	// We explicitly want people to be able to connect from anywhere
	CheckOrigin: func(r *http.Request) bool { return true },
}

func (s *Server) streamHandler(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
		return
	}

	s.broker.AddConnection(conn)
}

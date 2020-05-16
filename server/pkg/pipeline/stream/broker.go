package stream

import (
	"encoding/json"
	"log"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/pipeline"
	"github.com/gorilla/websocket"
)

// Broker represents the message broker used for streaming data
// messages to clients.
type Broker struct {
	clients    map[*client]bool
	broadcast  chan []byte
	register   chan *client
	unregister chan *client
	next       pipeline.Pipeline
}

// NewBroker creates a new Broker instance.
func NewBroker() *Broker {
	b := &Broker{
		clients:    make(map[*client]bool),
		broadcast:  make(chan []byte),
		register:   make(chan *client),
		unregister: make(chan *client),
	}

	go b.mainLoop()

	return b
}

func (b *Broker) mainLoop() {
	log.Printf("Starting broker mainLoop")
	for {
		select {

		case client := <-b.register:
			b.clients[client] = true
			log.Printf("Register client from %v", client.conn.RemoteAddr())

		case client := <-b.unregister:
			if _, ok := b.clients[client]; ok {
				delete(b.clients, client)
				close(client.send)
				log.Printf("Unregister client from %v", client.conn.RemoteAddr())
			}

		case message := <-b.broadcast:
			for client := range b.clients {
				select {
				case client.send <- message:
				default:
					close(client.send)
					delete(b.clients, client)
				}
			}
		}
	}
}

// AddConnection adds a new connection to the message broker.
func (b *Broker) AddConnection(conn *websocket.Conn) {
	b.register <- newClient(conn, b)
}

// Publish ...
func (b *Broker) Publish(m *model.Message) error {

	json, err := json.Marshal(m)
	if err != nil {
		return err
	}

	b.broadcast <- json

	if b.next != nil {
		return b.next.Publish(m)
	}
	return nil
}

// AddNext ...
func (b *Broker) AddNext(pe pipeline.Pipeline) {
	b.next = pe
}

// Next ...
func (b *Broker) Next() pipeline.Pipeline {
	return b.next
}

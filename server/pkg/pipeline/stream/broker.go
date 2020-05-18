package stream

import (
	"encoding/json"
	"log"
	"time"

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
	list       chan *listRequest
	next       pipeline.Pipeline
}

type listRequest struct {
	responseChannel chan *client
}

// NewBroker creates a new Broker instance.
func NewBroker() *Broker {
	b := &Broker{
		clients:    make(map[*client]bool),
		broadcast:  make(chan []byte, 64),
		register:   make(chan *client),
		unregister: make(chan *client),
		list:       make(chan *listRequest, 10),
	}

	go b.mainLoop()

	return b
}

func (b *Broker) mainLoop() {
	for {
		select {

		case client := <-b.register:
			b.clients[client] = true
			log.Printf("CONNECT WebSocket from %v", client.conn.RemoteAddr())

		case client := <-b.unregister:
			if _, ok := b.clients[client]; ok {
				delete(b.clients, client)
				close(client.send)
				log.Printf("DISCONNECT WebSocket from %v", client.conn.RemoteAddr())
			}

		case listRequest := <-b.list:
			for client := range b.clients {
				listRequest.responseChannel <- client
			}
			close(listRequest.responseChannel)

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

// ListClients lists clients connected via websocket streamer
func (b Broker) ListClients() []string {
	request := &listRequest{
		responseChannel: make(chan *client),
	}

	b.list <- request

	clients := []string{}

	for {
		select {
		case c, ok := <-request.responseChannel:
			if !ok {
				return clients
			}
			clients = append(clients, c.conn.RemoteAddr().String())

		case <-time.After(100 * time.Millisecond):
			return clients
		}
	}
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

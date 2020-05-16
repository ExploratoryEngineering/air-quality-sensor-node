package stream

import (
	"bytes"
	"log"
	"time"

	"github.com/gorilla/websocket"
)

type client struct {
	conn   *websocket.Conn
	send   chan []byte
	broker *Broker
}

const (
	sendQueueLen   = 256
	maxMessageSize = 256
	pongWait       = 60 * time.Second
	pingPeriod     = (pongWait * 9) / 10
	writeWait      = 10 * time.Second
)

var (
	newline = []byte{'\n'}
	space   = []byte{' '}
)

func newClient(conn *websocket.Conn, broker *Broker) *client {
	c := &client{
		conn:   conn,
		send:   make(chan []byte, sendQueueLen),
		broker: broker,
	}

	go c.readLoop()
	go c.writeLoop()

	return c
}

func (c *client) readLoop() {
	defer func() {
		c.broker.unregister <- c
		c.conn.Close()
	}()

	c.conn.SetReadLimit(maxMessageSize)
	c.conn.SetReadDeadline(time.Now().Add(pongWait))

	c.conn.SetPongHandler(func(string) error {
		c.conn.SetReadDeadline(time.Now().Add(pongWait))
		return nil
	})

	for {
		_, message, err := c.conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("Websocket error: %v", err)
			}
			break
		}
		message = bytes.TrimSpace(bytes.Replace(message, newline, space, -1))

		// Since we do not currently handle incoming messages we just log them
		log.Printf("Incoming message from [%v]: '%s'", c.conn.RemoteAddr(), message)
	}
}

func (c *client) writeLoop() {
	ticker := time.NewTicker(pingPeriod)
	defer func() {
		ticker.Stop()
		c.conn.Close()
	}()

	for {
		select {
		case message, ok := <-c.send:
			c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if !ok {
				// The broker closed the channel.
				c.conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}

			w, err := c.conn.NextWriter(websocket.TextMessage)
			if err != nil {
				log.Printf("Error writing to websocket [%v]: %v", c.conn.RemoteAddr(), err)
				return
			}
			w.Write(message)

			// Add queued chat messages to the current websocket message.
			n := len(c.send)
			for i := 0; i < n; i++ {
				w.Write(newline)
				w.Write(<-c.send)
			}

			if err := w.Close(); err != nil {
				log.Printf("Error closing websocket writer for [%v]: %v", c.conn.RemoteAddr(), err)
				return
			}
		case <-ticker.C:
			c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				log.Printf("Error writing ping message to [%v]: %v", c.conn.RemoteAddr(), err)
				return
			}
		}
	}
}

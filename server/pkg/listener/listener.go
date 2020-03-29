package listener

// Listener interface
type Listener interface {
	Start() error
	Shutdown()
	WaitForShutdown()
}

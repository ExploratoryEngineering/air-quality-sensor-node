package sqlitestore

import (
	"os"
	"sync"
	"log"

	"github.com/jmoiron/sqlx"
	_ "github.com/mattn/go-sqlite3" // Load sqlite3 driver
)

// SqliteStore implements the store interface with Sqlite
type SqliteStore struct {
	mu sync.Mutex
	db *sqlx.DB
}

// New creates new Store backed by SQLite3
func New(dbFile string) (*SqliteStore, error) {
	var databaseFileExisted = false
	if _, err := os.Stat(dbFile); err == nil {
		databaseFileExisted = true
	}

	// Turn on write-ahead log journaling annd turn off mutex
	// since we don't trust this to work anyway.
	cs := dbFile + "?" + "_journal=WAL&_mutex=no"

	d, err := sqlx.Open("sqlite3", cs)
	if err != nil {
		return nil, err
	}

	if err = d.Ping(); err != nil {
		return nil, err
	}

	if !databaseFileExisted {
		log.Printf("Creating database schema in %s", dbFile)
		createSchema(d, cs)
	}

	return &SqliteStore{db: d}, nil
}

// Close ...
func (s *SqliteStore) Close() error {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.db.Close()
}

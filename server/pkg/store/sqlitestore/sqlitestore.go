package sqlitestore

import (
	"os"
	"sync"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
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

	d, err := sqlx.Open("sqlite3", dbFile)
	if err != nil {
		return nil, err
	}

	if err = d.Ping(); err != nil {
		return nil, err
	}

	if !databaseFileExisted {
		createSchema(d, dbFile)
	}

	return &SqliteStore{db: d}, nil
}

// PutDevice ...
func (s *SqliteStore) PutDevice(d *model.Device) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	_, err := s.db.NamedExec("INSERT INTO devices (id,name,collection_id) VALUES(:id, :name, :collection_id)", d)
	return err
}

// GetDevice ...
func (s *SqliteStore) GetDevice(id string) (*model.Device, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var d model.Device
	err := s.db.QueryRowx("SELECT * FROM devices WHERE id = ?", id).StructScan(&d)
	if err != nil {
		return nil, err
	}

	return &d, nil
}

// UpdateDevice ...
func (s *SqliteStore) UpdateDevice(d *model.Device) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	_, err := s.db.NamedExec("UPDATE devices SET name = :name, collection_id = :collection_id WHERE id = :id", d)
	return err
}

// DeleteDevice ...
func (s *SqliteStore) DeleteDevice(id string) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	_, err := s.db.Exec("DELETE FROM devices WHERE id = ?", id)
	return err
}

// PutMessage ...
func (s *SqliteStore) PutMessage(m *model.Message) error {
	// TODO(borud): implement
	return nil
}

// GetMessage ...
func (s *SqliteStore) GetMessage(id int64) (*model.Message, error) {
	// TODO(borud): implement
	return nil, nil
}

// ListMessages ...
func (s *SqliteStore) ListMessages(offset int, limit int) ([]model.Message, error) {
	// TODO(borud): implement
	return nil, nil
}

// PutCal ...
func (s *SqliteStore) PutCal(c *model.Cal) error {
	// TODO(borud): implement
	return nil
}

// GetCal ...
func (s *SqliteStore) GetCal(id int64) (*model.Cal, error) {
	// TODO(borud): implement
	return nil, nil
}

// DeleteCal ...
func (s *SqliteStore) DeleteCal(id int64) error {
	// TODO(borud): implement
	return nil
}

// ListCal ...
func (s *SqliteStore) ListCal(deviceID string) ([]model.Cal, error) {
	// TODO(borud): implement
	return nil, nil
}

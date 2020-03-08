package sqlitestore

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

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

package sqlitestore

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

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

// ListDevices ...
func (s *SqliteStore) ListDevices(offset int, limit int) ([]model.Device, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var devices []model.Device
	err := s.db.Select(&devices, "SELECT * FROM devices ORDER BY id LIMIT ? OFFSET ?", limit, offset)
	return devices, err
}

package sqlitestore

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// PutCal ...
func (s *SqliteStore) PutCal(c *model.Cal) (int64, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	r, err := s.db.NamedExec("INSERT INTO cal (device_id,valid_from,valid_to) VALUES(:device_id, :valid_from, :valid_to)", c)
	if err != nil {
		return -1, err
	}
	return r.LastInsertId()
}

// GetCal ...
func (s *SqliteStore) GetCal(id int64) (*model.Cal, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var c model.Cal
	err := s.db.QueryRowx("SELECT * FROM cal WHERE id = ?", id).StructScan(&c)
	if err != nil {
		return nil, err
	}
	return &c, nil
}

// UpdateCal ...
func (s *SqliteStore) UpdateCal(c *model.Cal) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	_, err := s.db.NamedExec("UPDATE cal SET valid_from = :valid_from, valid_to = :valid_to WHERE id = :id", c)
	return err
}

// DeleteCal ...
func (s *SqliteStore) DeleteCal(id int64) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	_, err := s.db.Exec("DELETE FROM cal WHERE id = ?", id)
	return err
}

// ListCals ...
func (s *SqliteStore) ListCals(offset int, limit int) ([]model.Cal, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var cals []model.Cal
	err := s.db.Select(&cals, "SELECT * FROM cal ORDER BY device_id, valid_from ASC LIMIT ? OFFSET ?", limit, offset)
	return cals, err
}

// ListCalsForDevice ...
func (s *SqliteStore) ListCalsForDevice(deviceID string) ([]model.Cal, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var cals []model.Cal
	err := s.db.Select(&cals, "SELECT * FROM cal WHERE device_id = ? ORDER BY valid_from DESC", deviceID)
	return cals, err
}

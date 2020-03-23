package sqlitestore

import (
	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// PutCal ...
func (s *SqliteStore) PutCal(c *model.Cal) (int64, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	r, err := s.db.NamedExec(`
INSERT INTO cal
(
  device_id,
  valid_from,
  valid_to,
  afe_serial,
  afe_cal_date,
  vt20_offset,
  sensor1_we_e,
  sensor1_we_0,
  sensor1_ae_e,
  sensor1_ae_0,
  sensor1_pcb_gain, 
  sensor1_we_sensitivity, 
  sensor2_we_e,
  sensor2_we_0,
  sensor2_ae_e,          
  sensor2_ae_0,         
  sensor2_pcb_gain,
  sensor2_we_sensitivity,
  sensor3_we_e,
  sensor3_we_0,
  sensor3_ae_e,
  sensor3_ae_0,
  sensor3_pcb_gain,
  sensor3_we_sensitivity
)
VALUES(
  :device_id,
  :valid_from,
  :valid_to,
  :afe_serial,
  :afe_cal_date,
  :vt20_offset,
  :sensor1_we_e,
  :sensor1_we_0,
  :sensor1_ae_e,
  :sensor1_ae_0,
  :sensor1_pcb_gain, 
  :sensor1_we_sensitivity, 
  :sensor2_we_e,
  :sensor2_we_0,
  :sensor2_ae_e,          
  :sensor2_ae_0,         
  :sensor2_pcb_gain,
  :sensor2_we_sensitivity,
  :sensor3_we_e,
  :sensor3_we_0,
  :sensor3_ae_e,
  :sensor3_ae_0,
  :sensor3_pcb_gain,
  :sensor3_we_sensitivity)
`, c)
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

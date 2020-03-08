package sqlitestore

import (
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// PutMessage ...
func (s *SqliteStore) PutMessage(m *model.Message) (int64, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	// Pretty it ain't :-)
	r, err := s.db.NamedExec(`
  INSERT INTO messages
    (device_id,
     received,
     packetsize,
     sysid,
     firmware_ver,
     uptime,
     boardtemp,
     boardhum,
     status,
     gpstimestamp,
     lon,
     lat,
     alt,
     sensor1work,
     sensor1aux,
     sensor2work,
     sensor2aux,
     sensor3work,
     sensor3aux,
     afe3temp,
     opcpma,
     opcpmb,
     opcpmc,
     opcsampleperiod,
     opcsampleflowrate,
     opctemp,
     opchum,
     opcfanrevcount,
     opclaserstatus,
     opcbin_0,
     opcbin_1,
     opcbin_2,
     opcbin_3,
     opcbin_4,
     opcbin_5,
     opcbin_6,
     opcbin_7,
     opcbin_8,
     opcbin_9,
     opcbin_10,
     opcbin_11,
     opcbin_12,
     opcbin_13,
     opcbin_14,
     opcbin_15,
     opcbin_16,
     opcbin_17,
     opcbin_18,
     opcbin_19,
     opcbin_20,
     opcbin_21,
     opcbin_22,
     opcbin_23,
     opcsamplevalid)
    VALUES (:device_id,
            :received,
            :packetsize,
            :sysid,
            :firmware_ver,
            :uptime,
            :boardtemp,
            :boardhum,
            :status,
            :gpstimestamp,
            :lon,
            :lat,
            :alt,
            :sensor1work,
            :sensor1aux,
            :sensor2work,
            :sensor2aux,
            :sensor3work,
            :sensor3aux,
            :afe3temp,
            :opcpma,
            :opcpmb,
            :opcpmc,
            :opcsampleperiod,
            :opcsampleflowrate,
            :opctemp,
            :opchum,
            :opcfanrevcount,
            :opclaserstatus,
            :opcbin_0,
            :opcbin_1,
            :opcbin_2,
            :opcbin_3,
            :opcbin_4,
            :opcbin_5,
            :opcbin_6,
            :opcbin_7,
            :opcbin_8,
            :opcbin_9,
            :opcbin_10,
            :opcbin_11,
            :opcbin_12,
            :opcbin_13,
            :opcbin_14,
            :opcbin_15,
            :opcbin_16,
            :opcbin_17,
            :opcbin_18,
            :opcbin_19,
            :opcbin_20,
            :opcbin_21,
            :opcbin_22,
            :opcbin_23,
            :opcsamplevalid)`, m)
	if err != nil {
		return -1, err
	}
	return r.LastInsertId()
}

// GetMessage ...
func (s *SqliteStore) GetMessage(id int64) (*model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var m model.Message
	err := s.db.QueryRowx("SELECT * FROM messages WHERE id = ?", id).StructScan(&m)
	if err != nil {
		return nil, err
	}
	return &m, nil
}

// ListMessages ...
func (s *SqliteStore) ListMessages(offset int, limit int) ([]model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var msgs []model.Message
	err := s.db.Select(&msgs, "SELECT * FROM messages ORDER BY received DESC LIMIT ? OFFSET ?", limit, offset)
	return msgs, err
}

// ListMessagesByDate ...
func (s *SqliteStore) ListMessagesByDate(from time.Time, to time.Time) ([]model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var msgs []model.Message
	err := s.db.Select(&msgs, "SELECT * FROM messages WHERE received >= ? AND received < ? ORDER BY received", from, to)
	return msgs, err
}

// ListDeviceMessagesByDate ...
func (s *SqliteStore) ListDeviceMessagesByDate(deviceID string, from time.Time, to time.Time) ([]model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var msgs []model.Message
	err := s.db.Select(&msgs, "SELECT * FROM messages WHERE device_id = ? AND received >= ? AND received < ? ORDER BY received", deviceID, from, to)
	return msgs, err
}

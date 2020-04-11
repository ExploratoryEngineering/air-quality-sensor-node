package sqlitestore

import (
	"math"
	"time"

	"github.com/ExploratoryEngineering/air-quality-sensor-node/server/pkg/model"
)

// cleanFloat takes care of normalizing floats that are infinite or
// not a number to 0.0.  This eliminates the need for doing
// complicated NULL handling.
func cleanFloat(f *float64) {
	if math.IsNaN(*f) {
		*f = 0.0
		return
	}

	if math.IsInf(*f, 0) {
		*f = 0.0
		return
	}
}

// PutMessage ...
func (s *SqliteStore) PutMessage(m *model.Message) (int64, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	// TODO(borud): If these values are not cleaned up they will
	// result in NULLs and NaNs in the database.
	cleanFloat(&m.NO2PPB)
	cleanFloat(&m.O3PPB)
	cleanFloat(&m.NOPPB)
	cleanFloat(&m.AFE3TempValue)

	// Pretty it ain't :-)
	r, err := s.db.NamedExec(`
  INSERT INTO messages
    (device_id,
     received_time,
     packetsize,
     sysid,
     firmware_ver,
     uptime,
     boardtemp,
     board_rel_hum,
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
     afe3_temp_raw,

     no2_ppb,
     o3_ppb,
     no_ppb,
     afe3_temp_value,

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
            :received_time,
            :packetsize,
            :sysid,
            :firmware_ver,
            :uptime,
            :boardtemp,
            :board_rel_hum,
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
            :afe3_temp_raw,
            :no2_ppb,
            :o3_ppb,
            :no_ppb,
            :afe3_temp_value,
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
	err := s.db.Select(&msgs, "SELECT * FROM messages ORDER BY received_time DESC LIMIT ? OFFSET ?", limit, offset)
	return msgs, err
}

// ListMessagesByDate ...
func (s *SqliteStore) ListMessagesByDate(from time.Time, to time.Time) ([]model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var msgs []model.Message
	err := s.db.Select(&msgs, "SELECT * FROM messages WHERE received_time >= ? AND received_time < ? ORDER BY received_time", from, to)
	return msgs, err
}

// ListDeviceMessagesByDate ...
func (s *SqliteStore) ListDeviceMessagesByDate(deviceID string, from time.Time, to time.Time) ([]model.Message, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	var msgs []model.Message
	err := s.db.Select(&msgs, "SELECT * FROM messages WHERE device_id = ? AND received_time >= ? AND received_time < ? ORDER BY received_time", deviceID, from, to)
	return msgs, err
}

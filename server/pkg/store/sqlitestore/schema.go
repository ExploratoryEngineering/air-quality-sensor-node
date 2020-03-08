package sqlitestore

import (
	"fmt"
	"log"
	"strings"

	"github.com/jmoiron/sqlx"
)

const schema = `
-- PRAGMA foreign_keys = ON;
-- PRAGMA defer_foreign_keys = FALSE;

CREATE TABLE IF NOT EXISTS devices (
  id            TEXT PRIMARY KEY NOT NULL,
  name          TEXT NOT NULL,
  collection_id TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS cal (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  device_id   TEXT NOT NULL,
  valid_from   DATETIME NOT NULL,
  valid_to     DATETIME NOT NULL,

  FOREIGN KEY(device_id) REFERENCES devices(id)
);

CREATE TABLE IF NOT EXISTS messages (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  device_id     TEXT NOT NULL,
  received      DATETIME NOT NULL,
  packetsize    INTEGER NOT NULL,

  sysid         INTEGER NOT NULL,
  firmware_ver  INTEGER NOT NULL,
  uptime        INTEGER NOT NULL,
  boardtemp     REAL NOT NULL,
  boardhum      REAL NOT NULL,
  status        INTEGER NOT NULL,

  gpstimestamp  REAL NOT NULL,
  lon           REAL NOT NULL,
  lat           REAL NOT NULL,
  alt           REAL NOT NULL,

  sensor1work   INTEGER NOT NULL,
  sensor1aux    INTEGER NOT NULL,
  sensor2work   INTEGER NOT NULL,
  sensor2aux    INTEGER NOT NULL,
  sensor3work   INTEGER NOT NULL,
  sensor3aux    INTEGER NOT NULL,
  afe3temp      INTEGER NOT NULL,

  opcpma        INTEGER NOT NULL,
  opcpmb        INTEGER NOT NULL,
  opcpmc        INTEGER NOT NULL,

  opcsampleperiod   INTEGER NOT NULL,
  opcsampleflowrate INTEGER NOT NULL,
  opctemp           INTEGER NOT NULL,
  opchum            INTEGER NOT NULL,
  opcfanrevcount    INTEGER NOT NULL,
  opclaserstatus    INTEGER NOT NULL,
  opcbin_0          INTEGER NOT NULL,
  opcbin_1          INTEGER NOT NULL,
  opcbin_2          INTEGER NOT NULL,
  opcbin_3          INTEGER NOT NULL,
  opcbin_4          INTEGER NOT NULL,
  opcbin_5          INTEGER NOT NULL,
  opcbin_6          INTEGER NOT NULL,
  opcbin_7          INTEGER NOT NULL,
  opcbin_8          INTEGER NOT NULL,
  opcbin_9          INTEGER NOT NULL,
  opcbin_10         INTEGER NOT NULL,
  opcbin_11         INTEGER NOT NULL,
  opcbin_12         INTEGER NOT NULL,
  opcbin_13         INTEGER NOT NULL,
  opcbin_14         INTEGER NOT NULL,
  opcbin_15         INTEGER NOT NULL,
  opcbin_16         INTEGER NOT NULL,
  opcbin_17         INTEGER NOT NULL,
  opcbin_18         INTEGER NOT NULL,
  opcbin_19         INTEGER NOT NULL,
  opcbin_20         INTEGER NOT NULL,
  opcbin_21         INTEGER NOT NULL,
  opcbin_22         INTEGER NOT NULL,
  opcbin_23         INTEGER NOT NULL,
  opcsamplevalid    INTEGER NOT NULL,

  FOREIGN KEY(device_id) REFERENCES devices(id)

);

`

func createSchema(db *sqlx.DB, fileName string) {
	log.Printf("Creating database schema in %s", fileName)

	for n, statement := range strings.Split(schema, ";") {
		if _, err := db.Exec(statement); err != nil {
			panic(fmt.Sprintf("Statement %d failed: \"%s\" : %s", n+1, statement, err))
		}
	}
}

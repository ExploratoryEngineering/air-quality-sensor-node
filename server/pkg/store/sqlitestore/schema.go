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
  id   TEXT PRIMARY KEY NOT NULL,
);

CREATE TABLE IF NOT EXISTS cal (
);

CREATE TABLE IF NOT EXISTS samples (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  received      DATETIME NOT NULL,
  deviceid      TEXT NOT NULL,
  sysid         INTEGER NOT NULL,
  firmware      INTEGER NOT NULL,
  uptime        INTEGER NOT NULL,
  boardtemp     REAL NOT NULL,
  boardhumidity REAL NOT NULL,
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

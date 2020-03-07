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

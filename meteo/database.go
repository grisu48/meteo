/* Custom-made database backend for meteo */
package meteo

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

const db_filename = "meteo.db"

/* Station definition */
type Station struct {
	ID     int
	Name   string
	Fields []Field // Datafield names of this station
}

func CreateStation(id int, name string) Station {
	var s Station
	s.ID = id
	s.Name = name
	s.Fields = make([]Field, 0)
	return s
}

func (s1 *Station) compare(s2 Station) bool {
	if s1.ID != s2.ID || s1.Name != s2.Name {
		return false
	}
	n := len(s1.Fields)
	if n != len(s2.Fields) {
		return false
	}
	for i, f := range s1.Fields {
		if f != s2.Fields[i] {
			return false
		}
	}
	return true
}

type Field struct {
	Name string
	Unit string
}

func sanitize(v string) string {
	ret := strings.ReplaceAll(v, "|", "")
	ret = strings.ReplaceAll(ret, ",", "")
	return ret
}

//
func (f *Field) deserialize(value string) error {
	i := strings.Index(value, "|")
	if i < 0 {
		return fmt.Errorf("invalid field format")
	}
	f.Name = value[:i]
	if i < len(value) {
		f.Unit = value[i+1:]
	} else {
		f.Unit = ""
	}
	return nil
}

func (f *Field) serialize() string {
	return sanitize(f.Name) + "|" + sanitize(f.Unit)
}

type DataPoint struct {
	Station   int       // Station ID this datapoint belongs to
	Timestamp int64     // Timestamp
	Data      []float32 // data values that match Station.Fields
}

/* Database access */
type Database struct {
	dir         string    // Database folder
	db_filename string    // Path of the database filename
	Stations    []Station // Stations from the database
}

func fileExists(filename string) bool {
	if _, err := os.Stat(filename); os.IsNotExist(err) {
		return false
	}
	return true
}

func OpenDatabase(dir string) (Database, error) {
	db := Database{dir: dir}
	db.db_filename = fmt.Sprintf("%s%s%s", db.dir, string(os.PathSeparator), db_filename)
	if err := db.Read(); err != nil {
		return db, err
	}
	return db, nil
}

// Read stations from the database file
func (db *Database) Read() error {
	file, err := os.OpenFile(db.db_filename, os.O_RDONLY, 0600)
	if err != nil {
		if os.IsNotExist(err) {
			// File does not exist. We create the file and clean the internal structs
			db.Stations = make([]Station, 0)
			file.Close()
			// Create file
			if file, err = os.OpenFile(db.db_filename, os.O_WRONLY, 0640); err == nil {
				file.Close()
			}
			return nil
		} else {
			return err
		}
	}
	defer file.Close()
	db.Stations = make([]Station, 0)
	// Read line by line
	scanner := bufio.NewScanner(file)
	iLine := 0
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		iLine++
		split := strings.Split(line, ",")
		if len(split) < 2 {
			return fmt.Errorf("syntax error (line %d)", iLine)
		}
		station := Station{}
		station.ID, err = strconv.Atoi(split[0])
		if err != nil {
			return fmt.Errorf("id number format invalid (line %d)", iLine)
		}
		station.Name = split[1]
		station.Fields = make([]Field, 0)
		if len(split) > 2 {
			for _, value := range split[2:] {
				var field Field
				if err := field.deserialize(value); err != nil {
					return fmt.Errorf("%s (line %d)", err, iLine)
				}
				station.Fields = append(station.Fields, field)
			}
		}
		db.Stations = append(db.Stations, station)
	}
	return nil
}

// Write stations from the database file
func (db *Database) Write() error {
	file, err := os.OpenFile(db.db_filename, os.O_WRONLY|os.O_CREATE, 0600)
	if err != nil {
		return err
	}
	defer file.Close()
	for _, station := range db.Stations {
		line := fmt.Sprintf("%d,%s", station.ID, station.Name)
		for _, field := range station.Fields {
			line += "," + field.serialize()
		}
		line += "\n"
		if _, err := file.Write([]byte(line)); err != nil {
			return err
		}
	}
	return file.Sync()
}

func (db *Database) Close() error {
	return nil
}

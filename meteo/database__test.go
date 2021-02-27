package meteo

import (
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"
)

const testDBDirectory = "testDB123"

var db Database

func TestMain(m *testing.M) {
	// Initialisation
	fmt.Println("Initializing test run ... ")
	seed := time.Now().UnixNano()
	rand.Seed(seed)
	fmt.Printf("\tRandom seed %d\n", seed)

	var err error
	os.RemoveAll(testDBDirectory)
	os.Mkdir(testDBDirectory, 0755)
	db, err = OpenDatabase(testDBDirectory)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Error setting up database: ", err)
		os.Exit(1)
	}
	defer db.Close()

	// Run tests
	ret := m.Run()

	// Cleanup
	db.Close()
	os.RemoveAll(testDBDirectory)
	os.Exit(ret)
}

func TestStations(t *testing.T) {
	var err error

	if err = db.Read(); err != nil {
		t.Fatalf("Error reading database: %s", err)
		return
	}
	if len(db.Stations) != 0 {
		t.Fatalf("Database is not empty")
		return
	}

	// Add some stations
	station1 := CreateStation(1, "Station 1")
	station1.Fields = append(station1.Fields, Field{Name: "temperature", Unit: "deg C"})
	station1.Fields = append(station1.Fields, Field{Name: "humidity", Unit: "% rel"})
	station1.Fields = append(station1.Fields, Field{Name: "pressure", Unit: "hPa"})
	station2 := CreateStation(2, "Station 2")
	station2.Fields = append(station2.Fields, Field{Name: "temperature", Unit: "deg C"})
	station2.Fields = append(station2.Fields, Field{Name: "humidity", Unit: "% rel"})
	station2.Fields = append(station2.Fields, Field{Name: "pressure", Unit: "hPa"})
	station2.Fields = append(station2.Fields, Field{Name: "tVOC", Unit: ""})
	station3 := CreateStation(3, "Station 3")
	station3.Fields = append(station3.Fields, Field{Name: "distance", Unit: "km"})
	// Check compare operator
	if !station1.compare(station1) {
		t.Fatalf("Station compare operator failed (==)")
		return
	}
	if station1.compare(station2) {
		t.Fatalf("Station compare operator failed (!=)")
		return
	}
	db.Stations = append(db.Stations, station1, station2, station3)
	if err = db.Write(); err != nil {
		t.Fatalf("Error writing database: %s", err)
		return
	}
	// Ensure the stations are cleared from the database before reloading
	db.Stations = make([]Station, 0)
	if err = db.Read(); err != nil {
		t.Fatalf("Error re-reading database: %s", err)
		return
	}
	if len(db.Stations) != 3 {
		t.Fatalf("Database contains %d stations, but 3 are expected", len(db.Stations))
		return
	}
	// Check if stations match
	if !station1.compare(db.Stations[0]) {
		t.Fatalf("Station 1 match failure")
		return
	}
	if !station2.compare(db.Stations[1]) {
		t.Fatalf("Station 2 match failure")
		return
	}
	if !station3.compare(db.Stations[2]) {
		t.Fatalf("Station 3 match failure")
		return
	}
}

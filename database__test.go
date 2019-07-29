package main

import (
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"
)

var testDatabase = "_test_meteod_.db"
var db Persistence

func fileExists(filename string) bool {
	if _, err := os.Stat(filename); err == nil {
		return true
	} else if os.IsNotExist(err) {
		return false
	} else {
		return false		// Assume it's a zombie or something
	}
}

func TestMain(m *testing.M) {
	// Initialisation
	fmt.Println("Initializing test run ... ")
	seed := time.Now().UnixNano()
	rand.Seed(seed)
	fmt.Printf("\tRandom seed %d\n", seed)

	if fileExists(testDatabase) {
		fmt.Fprintln(os.Stderr, "Test file already exists: " + testDatabase)
		os.Exit(1)
	}
	var err error
	db, err = OpenDb(testDatabase)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Error setting up database: ", err)
		os.Exit(1)
	}

	// Run tests
	ret := m.Run()

	// Cleanup
	db.Close()
	os.Remove(testDatabase)
	os.Exit(ret)
}

func checkStation(station Station) (bool) {
	ref := station
	sec, err := db.GetStation(station.Id)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Database error: %s\n", err )
		return false
	}
	return ref == sec
}

func TestStations(t *testing.T) {
	t.Log("Setting up database")
	err := db.Prepare()
	if err != nil {
		t.Fatal("Error preparing database")
	}

	t.Log("Ensure database is empty")
	stations, err := db.GetStations()
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(stations) > 0 { t.Error(fmt.Sprintf("Fetched %d stations from expected empty database", len(stations))) }

	// Insert stations
	t.Log("Inserting stations")
	station1 := Station{Id:0, Name: "Station 1", Description:"First test station"}
	station2 := Station{Id:0, Name: "Station 2", Description:"Second test station"}

	err = db.InsertStation(&station1)
	if err != nil { t.Fatal("Database error: ", err ) }
	stations, err = db.GetStations()
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(stations) != 1 { t.Error(fmt.Sprintf("Fetched %d stations but expected 1", len(stations))) }
	if !checkStation(station1) {
		t.Error("Comparison after inserting station 1 failed")
	}
	err = db.InsertStation(&station2)
	if err != nil { t.Fatal("Database error: ", err ) }
	stations, err = db.GetStations()
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(stations) != 2 { t.Error(fmt.Sprintf("Fetched %d stations but expected 2", len(stations))) }
	if !checkStation(station1) {
		t.Error("Comparison after inserting station 2 failed")
	}

	t.Log("Checking for existing stations")
	exists, err := db.ExistsStation(station1.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if !exists { t.Error("Station 1 reported as not existing")}
	exists, err = db.ExistsStation(station2.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if !exists { t.Error("Station 2 reported as not existing")}
	exists, err = db.ExistsStation(station1.Id+station2.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if exists { t.Error("Station 1+2 reported as existing")}



}

func TestTokens(t *testing.T) {
	err := db.Prepare()
	if err != nil {
		t.Fatal("Error preparing database")
	}


}
package main

import (
	"fmt"
	"log"
	"math/rand"
	"os"
	"testing"
	"time"
)

var testDatabase = "_test_meteod_.db"
var db Persistence

func randomInt(min int, max int) int {
	if min > max {
		return randomInt(max, min)
	}
	if min == max {
		return min
	}
	return min + rand.Int()%(max-min)
}

func randomFloat32(min float32, max float32) float32 {
	if min > max {
		return randomFloat32(max, min)
	}
	if min == max {
		return min
	}
	return min + rand.Float32()*(max-min)
}

func randomString(n int) string {
	var letterRunes = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
	b := make([]rune, n)
	for i := range b {
		b[i] = letterRunes[rand.Intn(len(letterRunes))]
	}
	return string(b)
}

func fileExists(filename string) bool {
	if _, err := os.Stat(filename); err == nil {
		return true
	} else if os.IsNotExist(err) {
		return false
	} else {
		return false // Assume it's a zombie or something
	}
}

func TestMain(m *testing.M) {
	// Initialisation
	fmt.Println("Initializing test run ... ")
	seed := time.Now().UnixNano()
	rand.Seed(seed)
	fmt.Printf("\tRandom seed %d\n", seed)

	if fileExists(testDatabase) {
		fmt.Fprintln(os.Stderr, "Test file already exists: "+testDatabase)
		os.Exit(1)
	}
	fmt.Printf("\tFilename: %s\n", testDatabase)
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

func checkStation(station Station) bool {
	ref := station
	sec, err := db.GetStation(station.Id)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Database error: %s\n", err)
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
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(stations) > 0 {
		t.Error(fmt.Sprintf("Fetched %d stations from expected empty database", len(stations)))
	}

	// Insert stations
	t.Log("Inserting stations")
	station1 := Station{Id: 0, Name: "Station 1", Description: "First test station"}
	station2 := Station{Id: 0, Name: "Station 2", Description: "Second test station"}

	err = db.InsertStation(&station1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	stations, err = db.GetStations()
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(stations) != 1 {
		t.Error(fmt.Sprintf("Fetched %d stations but expected 1", len(stations)))
	}
	if !checkStation(station1) {
		t.Error("Comparison after inserting station 1 failed")
	}
	err = db.InsertStation(&station2)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	stations, err = db.GetStations()
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(stations) != 2 {
		t.Error(fmt.Sprintf("Fetched %d stations but expected 2", len(stations)))
	}
	if !checkStation(station1) {
		t.Error("Comparison after inserting station 2 failed")
	}

	t.Log("Checking for existing stations")
	exists, err := db.ExistsStation(station1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if !exists {
		t.Error("Station 1 reported as not existing")
	}
	exists, err = db.ExistsStation(station2.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if !exists {
		t.Error("Station 2 reported as not existing")
	}
	exists, err = db.ExistsStation(station1.Id + station2.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if exists {
		t.Error("Station 1+2 reported as existing")
	}

	t.Log("Inserting station with certain ID")
	station3 := Station{Id: 55, Name: "Station 55"}
	err = db.InsertStation(&station3)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if station3.Id != 55 {
		t.Fatal("Inserting station_55 with id 55, but ID has been reset")
	}
	if !checkStation(station3) {
		t.Error("Comparison after inserting station 55 failed")
	}

	t.Log("Updating station information")
	station3.Name = "Station 55_1"
	station3.Location = "Nowhere"
	station3.Description = "Updated description"
	err = db.UpdateStation(station3)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	station3_clone, err := db.GetStation(station3.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if station3 != station3_clone {
		t.Error("Comparison after updating station failed")
	}
	if station3_clone.Name != "Station 55_1" {
		t.Error("Update station name failed")
	}
	if station3_clone.Location != "Nowhere" {
		t.Error("Update station location failed")
	}
	if station3_clone.Description != "Updated description" {
		t.Error("Update station description failed")
	}

}

func TestDatapoints(t *testing.T) {
	progressionPoints := 100

	t.Log("Setting up database")
	err := db.Prepare()
	if err != nil {
		t.Fatal("Error preparing database")
	}

	t.Log("Inserting test stations")
	station1 := Station{Id: 0, Name: "Station 1", Description: "First test station"}
	station2 := Station{Id: 0, Name: "Station 2", Description: "Second test station"}
	err = db.InsertStation(&station1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	err = db.InsertStation(&station2)
	if err != nil {
		t.Fatal("Database error: ", err)
	}

	dps, err := db.GetLastDataPoints(station1.Id, 1000)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 0 {
		t.Fatal("Station 1 contains datapoints, when being created")
	}
	dps, err = db.GetLastDataPoints(station2.Id, 1000)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 0 {
		t.Fatal("Station 2 contains datapoints, when being created")
	}

	t.Log("Inserting test datapoints")
	now := time.Now().Unix()
	dp := DataPoint{
		Timestamp:   now - 100,
		Station:     station1.Id,
		Temperature: 12,
		Humidity:    69,
		Pressure:    94501,
	}
	err = db.InsertDataPoint(dp)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	dps, err = db.GetLastDataPoints(station1.Id, 1000)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 1 {
		t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when expecting 1", len(dps)))
	}
	if dps[0] != dp {
		t.Error("Fetched datapoint doesn't match inserted datapoint")
	}
	dps, err = db.GetLastDataPoints(station2.Id, 1000)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 0 {
		t.Fatal("Station 2 contains datapoints, but is expected to be empty")
	}

	dp = DataPoint{
		Timestamp:   now + 10,
		Station:     station1.Id,
		Temperature: 13,
		Humidity:    68,
		Pressure:    94500,
	}
	err = db.InsertDataPoint(dp)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	dps, err = db.GetLastDataPoints(station1.Id, 1000)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 2 {
		t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when expecting 2", len(dps)))
	}
	dps, err = db.GetLastDataPoints(station1.Id, 1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 1 {
		t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when only fetching 1", len(dps)))
	}
	if dps[0] != dp {
		t.Error("Fetched datapoint doesn't match inserted datapoint")
	}

	t.Log("Make a random progression on station 2")
	t0 := now - 10000
	dp = DataPoint{
		Timestamp:   t0,
		Station:     station2.Id,
		Temperature: randomFloat32(5, 30),
		Humidity:    randomFloat32(40, 80),
		Pressure:    randomFloat32(101300-500, 101300+500),
	}
	err = db.InsertDataPoint(dp)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	for i := 0; i < progressionPoints-1; i++ {
		dp.Timestamp += int64(randomFloat32(1, 10))
		dp.Temperature += randomFloat32(-2, 2)
		dp.Humidity += randomFloat32(-1, 1)
		dp.Pressure += randomFloat32(-50, 50)
		err = db.InsertDataPoint(dp)
		if err != nil {
			t.Fatal("Database error: ", err)
		}
	}
	t.Log("Fetching queries on station 2")
	dps, err = db.QueryStation(station2.Id, now-10000, dp.Timestamp, int64(progressionPoints*2), 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != progressionPoints {
		t.Error(fmt.Sprintf("Querystation yielded %d points, but %d points are expected", len(dps), progressionPoints))
	}
	dp1 := dps[1]
	dps, err = db.QueryStation(station2.Id, dps[2].Timestamp, dps[5].Timestamp, 10, 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 4 {
		t.Error(fmt.Sprintf("Querystation[2] yielded %d points, but %d points are expected", len(dps), 4))
	}

	last, err := db.GetLastDataPoints(station2.Id, 1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	t_l := last[0].Timestamp
	dps, err = db.QueryStation(station2.Id, t0, t_l, 2, 1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(dps) != 2 {
		t.Error(fmt.Sprintf("Querystation[3] yielded %d points, but %d points are expected", len(dps), 2))
	}
	if dps[0] != dp1 {
		t.Error("Offset 1 doesn't returned dp[1]")
	}
}

func checkToken(token Token) bool {
	tok1, err := db.GetToken(token.Token)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Database error: ", err)
		return false
	}
	return token == tok1
}

func TestTokens(t *testing.T) {
	t.Log("Setting up database")
	err := db.Prepare()
	if err != nil {
		t.Fatal("Error preparing database")
	}

	t.Log("Inserting test stations")
	station1 := Station{Id: 0, Name: "Station 1", Description: "First test station"}
	station2 := Station{Id: 0, Name: "Station 2", Description: "Second test station"}
	err = db.InsertStation(&station1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	err = db.InsertStation(&station2)
	if err != nil {
		t.Fatal("Database error: ", err)
	}

	tok1 := Token{Token: randomString(32), Station: station1.Id}
	err = db.InsertToken(tok1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	tok2 := Token{Token: randomString(32), Station: station2.Id}
	err = db.InsertToken(tok2)
	if err != nil {
		t.Fatal("Database error: ", err)
	}

	tokens, err := db.GetStationTokens(station1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(tokens) != 1 {
		t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 1", len(tokens)))
	}
	if !checkToken(tok1) {
		fmt.Errorf("Checktoken failed (token1) ")
	}
	tokens, err = db.GetStationTokens(station2.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(tokens) != 1 {
		t.Error(fmt.Sprintf("Fetched %d tokens from station 2, but expected 1", len(tokens)))
	}
	if !checkToken(tok2) {
		fmt.Errorf("Checktoken failed (token2) ")
	}

	// Test remove tokens
	err = db.RemoveToken(tok1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	tokens, err = db.GetStationTokens(station1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(tokens) != 0 {
		t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 0", len(tokens)))
	}
	tokens, err = db.GetStationTokens(station2.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(tokens) != 1 {
		t.Error(fmt.Sprintf("Fetched %d tokens from station 2, but expected 1", len(tokens)))
	}
	err = db.InsertToken(tok1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	tokens, err = db.GetStationTokens(station1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(tokens) != 1 {
		t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 1", len(tokens)))
	}
	if !checkToken(tok1) {
		fmt.Errorf("Checktoken failed (token1) ")
	}
	if !checkToken(tok2) {
		fmt.Errorf("Checktoken failed (token2) ")
	}
}

func TestLightnings(t *testing.T) {
	seriesPoints := 100

	lightnings, err := db.GetLightnings(1000, 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(lightnings) != 0 {
		t.Error("Nonempty lightning table")
	}

	now := time.Now().Unix()

	t.Log("Inserting first lightning")
	lightning := Lightning{Timestamp: now, Station: 1, Distance: 0}
	err = db.InsertLightning(lightning)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	lightnings, err = db.GetLightnings(1000, 0)
	if len(lightnings) != 1 {
		t.Error("Failed to fetch first lightning")
	}
	if lightnings[0] != lightning {
		t.Error("First lightning mismatched")
	}

	t.Log("Inserting lightning at same time")
	err = db.InsertLightning(Lightning{Timestamp: now, Station: 2, Distance: 1.024})
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	lightnings, err = db.GetLightnings(1000, 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(lightnings) != 2 {
		t.Error("Failed to fetch second lightning")
	}
	// Although now explicitly required for now the lightnings preserve the last-in first-out order, so this is fine
	if lightnings[1].Timestamp != now {
		t.Error("Second lightning Timestamp mismatched")
	}
	if lightnings[1].Station != 2 {
		t.Error("Second lightning Station mismatched")
	}
	if lightnings[1].Distance != 1.024 {
		t.Error("Second lightning Distance mismatched")
	}

	t.Log("Inserting lightning series")
	for i := 0; i < seriesPoints; i++ {
		err = db.InsertLightning(Lightning{Timestamp: now + int64(i), Station: 3, Distance: 5.0 + float32(i)*0.1})
		if err != nil {
			t.Fatal("Database error: ", err)
		}
	}
	// Test to fetch all
	lightnings, err = db.GetLightnings(seriesPoints+2, 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(lightnings) != seriesPoints+2 {
		t.Errorf("Fetching %d lightnings returned %d", seriesPoints+2, len(lightnings))
	}

	// Test only the given last subseries
	lightnings, err = db.GetLightnings(seriesPoints, 0)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(lightnings) != seriesPoints {
		t.Errorf("Fetching %d lightnings returned %d", seriesPoints, len(lightnings))
	}
	for i := 0; i < seriesPoints; i++ {
		lightning := lightnings[i]

		i = seriesPoints - i - 1                  // i is here in reverse order
		timestamp := now + int64(i)               // Expected timestamp
		distance := float32(5.0 + float32(i)*0.1) // Expected distance
		fmt.Printf("%d %d %f\n", timestamp, 3, distance)
		if lightning.Station != 3 {
			t.Errorf("Lightning series Station mismatched (%d != %d)", lightning.Station, 3)
		}
		if lightning.Timestamp != timestamp {
			t.Errorf("Lightning series Timestamp mismatched (%d != %d)", lightning.Timestamp, timestamp)
		}
		if lightning.Distance != distance {
			t.Errorf("Lightning series Distance mismatched (%f != %f)", lightning.Distance, distance)
		}
	}
}

func TestOmbrometers(t *testing.T) {
	seriesPoints := 100

	ombrometers, err := db.GetOmbrometers()
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(ombrometers) != 0 {
		t.Error("Not empty ombrometer list at beginning")
	}
	ombrometer1 := Station{Name: "Ombrometer 1", Location: "Location 1", Description: "First ombrometer"}
	err = db.InsertOmbrometer(&ombrometer1)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	log.Printf("Inserted ombrometer 1 with ID %d", ombrometer1.Id)

	ombrometers, err = db.GetOmbrometers()
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(ombrometers) != 1 {
		t.Error("Got empty ombrometer after inserting one")
	}
	if ombrometer1 != ombrometers[0] {
		t.Error("First ombrometer mismatch")
	}
	if ombrometer1.Id != ombrometers[0].Id {
		t.Error("Ombrometer Id mismatch")
	}
	if ombrometer1.Name != ombrometers[0].Name {
		t.Error("Ombrometer Name mismatch")
	}
	if ombrometer1.Location != ombrometers[0].Location {
		t.Error("Ombrometer Location mismatch")
	}
	if ombrometer1.Description != ombrometers[0].Description {
		t.Error("Ombrometer Description mismatch")
	}

	ombrometer2 := Station{Name: "Ombrometer 2", Location: "Location 2", Description: "Second ombrometer"}
	err = db.InsertOmbrometer(&ombrometer2)
	log.Printf("Inserted ombrometer 2 with ID %d", ombrometer2.Id)
	ombrometers, err = db.GetOmbrometers()
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(ombrometers) != 2 {
		t.Errorf("Fetched %d ombrometers instead of two after inserting another one", len(ombrometers))
	}

	log.Println("Updating ombrometer station")
	ombrometer2.Name = "Updated Ombrometer 2"
	ombrometer2.Location = "New Location 2"
	ombrometer2.Description = "Improved second ombrometer"
	err = db.UpdateOmbrometer(ombrometer2)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	ombro2, err := db.getOmbrometer(ombrometer2.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if ombro2 != ombrometer2 {
		t.Error("Ombrometer 2 update failed")
	}

	log.Println("Test ombrometer readings")
	reading, err := db.GetOmbrometerLastReading(ombrometer1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if reading.Timestamp != 0 || reading.Millimeters != 0 {
		t.Error("Ombrometer 1 returned ghost reading")
	}
	readings, err := db.GetOmbrometerReadings(ombrometer1.Id, 10)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(readings) != 0 {
		t.Error("Ombrometer 1 returned ghost readings")
	}
	reading.Millimeters = 1
	reading.Timestamp = time.Now().Unix()
	reading.Station = ombrometer1.Id
	err = db.InsertRainMeasurement(reading)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	reading2, err := db.GetOmbrometerLastReading(ombrometer1.Id)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if reading2 != reading {
		t.Error("Ombrometer 1 returned false reading")
	}
	readings, err = db.GetOmbrometerReadings(ombrometer2.Id, 10)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(readings) != 0 {
		t.Error("Ombrometer 2 returned ghost readings")
	}
	reading.Station = ombrometer2.Id
	now := time.Now().Unix()
	for i := 0; i < seriesPoints; i++ {
		reading.Millimeters = float32(i) * 0.1
		reading.Timestamp = now + int64(i)
		err = db.InsertRainMeasurement(reading)
		if err != nil {
			t.Fatal("Database error: ", err)
		}
	}

	readings, err = db.GetOmbrometerReadings(ombrometer2.Id, seriesPoints)
	if err != nil {
		t.Fatal("Database error: ", err)
	}
	if len(readings) != seriesPoints {
		t.Errorf("Ombrometer 2 returned %d instead of %d readings", len(readings), seriesPoints)
	}
	// Check if expected values
	for i, reading := range readings {
		i = seriesPoints - i - 1 // Inverted output from database!
		if reading.Timestamp != now+int64(i) {
			t.Errorf("Reading %d timestamp mismtach: %d != %d", i, reading.Timestamp, now+int64(i))
		}
		if reading.Millimeters != float32(i)*0.1 {
			t.Errorf("Reading %d millimeters mismtach: %f != %f", i, reading.Millimeters, float32(i)*0.1)
		}
	}
	log.Println("Rain series OK")
}

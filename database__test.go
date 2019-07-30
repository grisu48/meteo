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



func randomInt(min int, max int) int {
	if min > max { return randomInt(max, min) }
	if min == max { return min }
	return min+rand.Int()%(max-min)
}

func randomFloat32(min float32, max float32) float32 {
	if min > max { return randomFloat32(max, min) }
	if min == max { return min }
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

func TestDatapoints(t *testing.T) {
	progressionPoints := 1000

	t.Log("Setting up database")
	err := db.Prepare()
	if err != nil {
		t.Fatal("Error preparing database")
	}

	t.Log("Inserting test stations")
	station1 := Station{Id:0, Name: "Station 1", Description:"First test station"}
	station2 := Station{Id:0, Name: "Station 2", Description:"Second test station"}
	err = db.InsertStation(&station1)
	if err != nil { t.Fatal("Database error: ", err ) }
	err = db.InsertStation(&station2)
	if err != nil { t.Fatal("Database error: ", err ) }

	dps, err := db.GetLastDataPoints(station1.Id, 1000)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 0 { t.Fatal("Station 1 contains datapoints, when being created")}
	dps, err = db.GetLastDataPoints(station2.Id, 1000)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 0 { t.Fatal("Station 2 contains datapoints, when being created")}


	t.Log("Inserting test datapoints")
	now := time.Now().Unix()
	dp := DataPoint{
		Timestamp:   now-100,
		Station:     station1.Id,
		Temperature: 12,
		Humidity:    69,
		Pressure:    94501,
	}
	err = db.InsertDataPoint(dp)
	if err != nil { t.Fatal("Database error: ", err ) }
	dps, err = db.GetLastDataPoints(station1.Id, 1000)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 1 { t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when expecting 1", len(dps))) }
	if dps[0] != dp { t.Error("Fetched datapoint doesn't match inserted datapoint") }
	dps, err = db.GetLastDataPoints(station2.Id, 1000)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 0 { t.Fatal("Station 2 contains datapoints, but is expected to be empty")}


	dp = DataPoint{
		Timestamp:   now+10,
		Station:     station1.Id,
		Temperature: 13,
		Humidity:    68,
		Pressure:    94500,
	}
	err = db.InsertDataPoint(dp)
	if err != nil { t.Fatal("Database error: ", err ) }
	dps, err = db.GetLastDataPoints(station1.Id, 1000)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 2 { t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when expecting 2", len(dps))) }
	dps, err = db.GetLastDataPoints(station1.Id, 1)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 1 { t.Fatal(fmt.Sprintf("Station 1 contains %d datapoints, when only fetching 1", len(dps))) }
	if dps[0] != dp { t.Error("Fetched datapoint doesn't match inserted datapoint") }

	t.Log("Make a random progression on station 2")
	t0 := now-10000
	dp = DataPoint{
		Timestamp:   t0,
		Station:     station2.Id,
		Temperature: randomFloat32(5,30),
		Humidity:    randomFloat32(40,80),
		Pressure:    randomFloat32(101300-500,101300+500),
	}
	err = db.InsertDataPoint(dp)
	if err != nil { t.Fatal("Database error: ", err ) }
	for i := 0; i<progressionPoints-1; i++ {
		dp.Timestamp += int64(randomFloat32(1,10))
		dp.Temperature += randomFloat32(-2,2)
		dp.Humidity += randomFloat32(-1,1)
		dp.Pressure += randomFloat32(-50, 50)
		err = db.InsertDataPoint(dp)
		if err != nil { t.Fatal("Database error: ", err ) }
	}
	t.Log("Fetching queries on station 2")
	dps, err = db.QueryStation(station2.Id, now-10000, dp.Timestamp, int64(progressionPoints*2), 0)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != progressionPoints { t.Error(fmt.Sprintf("Querystation yielded %d points, but %d points are expected", len(dps), progressionPoints)) }
	dp1 := dps[1]
	dps, err = db.QueryStation(station2.Id, dps[2].Timestamp, dps[5].Timestamp, 10, 0)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 4 { t.Error(fmt.Sprintf("Querystation[2] yielded %d points, but %d points are expected", len(dps), 4)) }

	last, err := db.GetLastDataPoints(station2.Id, 1)
	if err != nil { t.Fatal("Database error: ", err ) }
	t_l := last[0].Timestamp
	dps, err = db.QueryStation(station2.Id, t0, t_l, 2, 1)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(dps) != 2 { t.Error(fmt.Sprintf("Querystation[3] yielded %d points, but %d points are expected", len(dps), 2)) }
	if dps[0] != dp1 { t.Error("Offset 1 doesn't returned dp[1]") }
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
	station1 := Station{Id:0, Name: "Station 1", Description:"First test station"}
	station2 := Station{Id:0, Name: "Station 2", Description:"Second test station"}
	err = db.InsertStation(&station1)
	if err != nil { t.Fatal("Database error: ", err ) }
	err = db.InsertStation(&station2)
	if err != nil { t.Fatal("Database error: ", err ) }

	tok1 := Token{Token:randomString(32), Station:station1.Id}
	err = db.InsertToken(tok1)
	if err != nil { t.Fatal("Database error: ", err ) }
	tok2 := Token{Token:randomString(32), Station:station2.Id}
	err = db.InsertToken(tok2)
	if err != nil { t.Fatal("Database error: ", err ) }

	tokens, err := db.GetStationTokens(station1.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(tokens) != 1 { t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 1", len(tokens)))}
	if !checkToken(tok1) { fmt.Errorf("Checktoken failed (token1) ")}
	tokens, err = db.GetStationTokens(station2.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(tokens) != 1 { t.Error(fmt.Sprintf("Fetched %d tokens from station 2, but expected 1", len(tokens)))}
	if !checkToken(tok2) { fmt.Errorf("Checktoken failed (token2) ")}

	// Test remove tokens
	err = db.RemoveToken(tok1)
	if err != nil { t.Fatal("Database error: ", err ) }
	tokens, err = db.GetStationTokens(station1.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(tokens) != 0 { t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 0", len(tokens)))}
	tokens, err = db.GetStationTokens(station2.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(tokens) != 1 { t.Error(fmt.Sprintf("Fetched %d tokens from station 2, but expected 1", len(tokens)))}
	err = db.InsertToken(tok1)
	if err != nil { t.Fatal("Database error: ", err ) }
	tokens, err = db.GetStationTokens(station1.Id)
	if err != nil { t.Fatal("Database error: ", err ) }
	if len(tokens) != 1 { t.Error(fmt.Sprintf("Fetched %d tokens from station 1, but expected 1", len(tokens)))}
	if !checkToken(tok1) { fmt.Errorf("Checktoken failed (token1) ")}
	if !checkToken(tok2) { fmt.Errorf("Checktoken failed (token2) ")}
}


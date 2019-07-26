package main

import (
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"
)

var testDatabase = "_test_meteod_.db"

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
	db, err := OpenDb(testDatabase)
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

func Test1(t *testing.T) {

}
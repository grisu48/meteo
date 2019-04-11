package main

import "fmt"

var db Persistence

func main() {
	hostname := "localhost"
	username := "meteo"
	password := ""
	database := "meteo"
	port := 3306
	var err error

	db, err = ConnectDb(hostname, username, password, database, port)
	if err != nil {
		panic(err)
	}
	fmt.Println("Connected to database")
}

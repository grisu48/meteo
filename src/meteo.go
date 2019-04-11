package main

import "fmt"
import "github.com/BurntSushi/toml"

var db Persistence

type tomlConfig struct {
	DB        tomlDatabase  `toml:"database"`
	Webserver tomlWebserver `toml:"webserver"`
}

type tomlDatabase struct {
	hostname string
	username string
	password string
	database string
	port     int
}

type tomlWebserver struct {
	port int
}

func main() {
	var err error

	// Read config
	var cf tomlConfig
	cf.DB.hostname = "localhost"
	cf.DB.username = "meteo"
	cf.DB.password = ""
	cf.DB.database = "meteo"
	cf.DB.port = 3306
	if _, err := toml.DecodeFile("meteo.toml", &cf); err != nil {
		panic(err)
	}

	// Connect database
	db, err = ConnectDb(cf.DB.hostname, cf.DB.username, cf.DB.password, cf.DB.database, cf.DB.port)
	if err != nil {
		panic(err)
	}
	if err := db.Prepare(); err != nil {
		panic(err)
	}
	fmt.Println("Connected to database")

	// Setup webserver
}

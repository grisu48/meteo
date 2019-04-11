package main

import "fmt"
import "log"
import "net/http"
import "strconv"
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
	port     int
	bindaddr string
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
	cf.Webserver.port = 8802
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
	addr := cf.Webserver.bindaddr + ":" + strconv.Itoa(cf.Webserver.port)
	fmt.Println("Serving http://" + addr + "")
	http.HandleFunc("/", handler)
	log.Fatal(http.ListenAndServe(addr, nil))
}

func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "meteo\n")
}

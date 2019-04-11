package main

import "fmt"
import "log"
import "strings"
import "net/http"
import "strconv"
import "encoding/json"
import "io/ioutil"
import "time"
import "github.com/BurntSushi/toml"

var db Persistence

type tomlConfig struct {
	DB        tomlDatabase  `toml:"Database"`
	Webserver tomlWebserver `toml:"Webserver"`
}

type tomlDatabase struct {
	Hostname string
	Username string
	Password string
	Database string
	Port     int
}

type tomlWebserver struct {
	Port     int
	Bindaddr string
}


type jsonDataPoint struct {
	Token string
	Timestamp int64
	T float32
	Hum float32
	P float32
}

func main() {
	var err error

	// Read config
	var cf tomlConfig
	cf.DB.Hostname = "localhost"
	cf.DB.Username = "meteo"
	cf.DB.Password = ""
	cf.DB.Database = "meteo"
	cf.DB.Port = 3306
	cf.Webserver.Port = 8802
	if _, err := toml.DecodeFile("meteo.toml", &cf); err != nil {
		panic(err)
	}

	// Connect database
	db, err = ConnectDb(cf.DB.Hostname, cf.DB.Username, cf.DB.Password, cf.DB.Database, cf.DB.Port)
	if err != nil {
		panic(err)
	}
	if err := db.Prepare(); err != nil {
		panic(err)
	}
	fmt.Println("Connected to database")

	// Setup webserver
	addr := cf.Webserver.Bindaddr + ":" + strconv.Itoa(cf.Webserver.Port)
	fmt.Println("Serving http://" + addr + "")
	http.HandleFunc("/", handler)
	http.HandleFunc("/stations", stationsHandler)
	http.HandleFunc("/station/", stationHandler)
	log.Fatal(http.ListenAndServe(addr, nil))
}

/* Function callback when received a datapoint. Return true if the datapoint is accepted
 * Accepted datapoint doesn't mean automatically pushed datapoint.
 */
func received(dp jsonDataPoint) bool {
	if dp.Token == "" { return false }		// No token - Ignore

	station, err := db.GetStationToken(dp.Token)
	if err != nil { panic(err) }
	if station <= 0 { return false }	// Denied
	
	exists, err := db.ExistsStation(station)
	if err != nil { panic(err) }
	if !exists {
		fmt.Printf("Station doesn't exists: %d\n", station)
		return false
	}

	// Set timestamp if not set by sender
	if dp.Timestamp == 0 {
		dp.Timestamp = time.Now().Unix()
	}
	
	fmt.Printf("RECV: %d at %d, t = %f, hum = %f, p = %f\n", station, dp.Timestamp, dp.T, dp.Hum, dp.P)
	return true
}


/* ==== Webserver =========================================================== */

func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "meteo\n")
}

func stationsHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method == "GET" {
		stations, err := db.GetStations()
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error"))
			panic(err)
		} else {
			fmt.Fprintf(w,"# Id,Name,Location,Description\n")
			for _, station := range stations {
				fmt.Fprintf(w, "%d,%s,%s,%s\n", station.Id, station.Name, station.Location, station.Description)
			}
		}
	} else if r.Method == "POST" {
		// Reserved for future
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("N/A\n"))
	} else {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad request\n"))
	}
}

func stationHandler(w http.ResponseWriter, r *http.Request) {
	// Get ID
	s_id := strings.TrimPrefix(r.URL.Path, "/station/")
	id, err := strconv.Atoi(s_id)
	if err != nil || id <= 0 {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad paramter: id\n"))
		return
	}

	if r.Method == "GET" {
		station, err := db.GetStation(id)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error\n"))
			panic(err)
		}
		if station.Id == 0 {
			w.WriteHeader(http.StatusNotFound)
			w.Write([]byte("Station not found\n"))
			return
		}

		// Get parameters
		params := r.URL.Query()
		limit := 100
		if val, ok := params["limit"]; ok {
			limit, _ = strconv.Atoi(val[0])
			if limit <= 0 {
				w.WriteHeader(http.StatusBadRequest)
				w.Write([]byte("Bad paramter: limit\n"))
				return
			}
			if limit > 1000 { limit = 1000 }
		}

		// Print station and datapoints
		datapoints, err := db.GetLastDataPoints(station.Id, limit)
		fmt.Fprintf(w, "# Station %d, %s, %s, %s\n", station.Id, station.Name, station.Location, station.Description)
		for _, dp := range datapoints {
			fmt.Fprintf(w, "%d,%f,%f,%f\n", dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
		}
		

	} else if r.Method == "POST" { // Push data
		// Determine type
		contentType := r.Header["Content-Type"][0]

		if contentType == "application/x-www-form-urlencoded" {
			// Plaintext
			body, err := ioutil.ReadAll(r.Body)
			if err != nil { panic(err) }
			s_body := string(body)
			
			if len(s_body) == 0 {
				w.WriteHeader(http.StatusBadRequest)
				w.Write([]byte("Empty header\n"))
			}
			
			// XXX Reserved for future 

			w.WriteHeader(http.StatusBadRequest)
			w.Write([]byte("N/A\n"))
		} else if contentType == "application/json" {
			decoder := json.NewDecoder(r.Body)
			var dp jsonDataPoint
			err := decoder.Decode(&dp)
			if err != nil {
				w.WriteHeader(http.StatusBadRequest)
				w.Write([]byte("Illegal json\n"))
				return
			}
			ok := received(dp)
			if ok {
				w.Write([]byte("OK\n"))
			} else {
				w.Write([]byte("DENIED\n"))
			}
		} else {
			w.WriteHeader(http.StatusBadRequest)
			w.Write([]byte("Unsupported Content-Type\n"))
			return
		}
	} else {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad request\n"))
	}
}

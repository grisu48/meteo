package main

import "fmt"
import "log"
import "strings"
import "net/http"
import "strconv"
import "encoding/json"
import "io/ioutil"
import "time"
import "math"
import "github.com/BurntSushi/toml"

type tomlConfig struct {
	DB        tomlDatabase  `toml:"Database"`
	Webserver tomlWebserver `toml:"Webserver"`
	PushDelay int64
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

var cf tomlConfig
var db Persistence


func main() {
	var err error 

	// Default config values
	cf.PushDelay = 60		// 60 Seconds
	cf.DB.Hostname = "localhost"
	cf.DB.Username = "meteo"
	cf.DB.Password = ""
	cf.DB.Database = "meteo"
	cf.DB.Port = 3306
	cf.Webserver.Port = 8802
	// Read config
	toml.DecodeFile("/etc/meteod.toml", &cf)
	toml.DecodeFile("meteod.toml", &cf)

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
	http.HandleFunc("/current", readingsHandler)
	http.HandleFunc("/latest", readingsHandler)
	http.HandleFunc("/readings", readingsHandler)
	http.HandleFunc("/query", queryHandler)
	log.Fatal(http.ListenAndServe(addr, nil))
}

func doDatapointPush(dp jsonDataPoint, station int) (bool, error) {
	datapoints, err := db.GetLastDataPoints(station, 1)
	if err != nil { return false, err }
	if len(datapoints) == 0 { return true, nil }
	last := datapoints[0]

	now := time.Now().Unix()
	lastTimestamp := last.Timestamp
	diff := now - lastTimestamp
	if diff < 0 { diff = -diff }

	// XXX Check time range maybe

	if diff > cf.PushDelay {
		return true,nil 
	} else {
		// Within the ignore interval, but check for significant deviations
		if math.Abs((float64)(last.Temperature - dp.T)) > 0.5 { return true, nil }
		if math.Abs((float64)(last.Pressure - dp.P)) > 100 { return true, nil }
		if math.Abs((float64)(last.Humidity - dp.Hum)) > 2 { return true, nil }
	}
	return false, nil
}

func isPlausible(dp jsonDataPoint) bool {
	if dp.T < -100 || dp.T > 1000 { return false }
	if dp.Hum < 0 || dp.Hum > 100 { return false }
	p_mbar := dp.P / 100.0
	if p_mbar < 10 { return false }
	if p_mbar > 2000 { return false }		// We are not a submarine!
	return true
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

	// Repair pressure unit, that could be in mbar but is expected in hPa
	if dp.P < 10000 { dp.P *= 100 }

	// Check ranges for plausability
	if !isPlausible(dp) {
		fmt.Printf("UNPLAUSIBLE: %d at %d, t = %f, hum = %f, p = %f\n", station, dp.Timestamp, dp.T, dp.Hum, dp.P)
		return true
	}

	push, err := doDatapointPush(dp, station)
	if err != nil { panic(err) }

	if push {
		db_dp := DataPoint{Timestamp: dp.Timestamp, Station: station, Temperature: dp.T, Humidity: dp.Hum, Pressure: dp.P}
		err := db.InsertDataPoint(db_dp)
		if err != nil { panic(err) }
		fmt.Printf("PUSH: %d at %d, t = %f, hum = %f, p = %f\n", station, dp.Timestamp, dp.T, dp.Hum, dp.P)
	} else {
		fmt.Printf("RECV: %d at %d, t = %f, hum = %f, p = %f\n", station, dp.Timestamp, dp.T, dp.Hum, dp.P)
	}
	
	return true
}


/* ==== Webserver =========================================================== */

func handler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "meteo Server\n")
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

func readingsHandler(w http.ResponseWriter, r *http.Request) {	
	if r.Method == "GET" {
		stations, err := db.GetStations()
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error"))
			panic(err)
		} else {
			fmt.Fprintf(w, "# Station, Timestamp, Temperature, Humidity, Pressure\n")
			for _, station := range stations {
				dps, err := db.GetLastDataPoints(station.Id, 1)
				if err != nil {
					w.WriteHeader(http.StatusInternalServerError)
					w.Write([]byte("Server error"))
					panic(err)
				}
				if len(dps) > 0 {
					dp := dps[0]
					fmt.Fprintf(w, "%d,%d,%.2f,%.2f,%.2f\n", station.Id, dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)					
				}
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
		fmt.Fprintf(w, "## Station %d: '%s' in %s, %s\n", station.Id, station.Name, station.Location, station.Description)
		fmt.Fprintf(w, "# Timestamp, Temperature, Humidity, Pressure\n")
		fmt.Fprintf(w, "# Seconds, degree C, %% rel, hPa\n")
		for _, dp := range datapoints {
			fmt.Fprintf(w, "%d,%.2f,%.2f,%.2f\n", dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
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

func v_str(values []string, emptyValue string) (string) {
	if len(values) == 0 {
		return emptyValue
	} else {
		return values[0]
	}
}

func v_l(values []string, emptyValue int64) (int64) {
	if len(values) == 0 { return emptyValue }
	// Take the first that works
	for _, v := range values {
		l, err := strconv.ParseInt(v, 10, 64)
		if err == nil { return l }
	}
	return emptyValue
}

func queryHandler(w http.ResponseWriter, r *http.Request) {	
	if r.Method == "GET" {
		values := r.URL.Query()
		id, err := strconv.Atoi(v_str(values["id"], ""))
		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			w.Write([]byte("Bad id\n"))
			return	
		}

		station, err := db.GetStation(id)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error\n"))
			panic(err)
		}
		if station.Id == 0 {
			w.WriteHeader(http.StatusBadRequest)
			w.Write([]byte("Station not found\n"))
			return	
		}

		limit := v_l(values["limit"], 1000)
		offset := v_l(values["offset"], 0)
		t_min := v_l(values["tmin"], 0)
		t_max := v_l(values["tmax"], time.Now().Unix())

		// Make values sane
		if limit < 0 || limit > 1000 { limit = 1000 }
		if offset < 0 { offset = 0 }

		datapoints, err := db.QueryStation(id, t_min, t_max, limit, offset)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error\n"))
			panic(err)
		}

		fmt.Fprintf(w, "## Station %d: '%s' in %s, %s\n", station.Id, station.Name, station.Location, station.Description)
		fmt.Fprintf(w, "# Timestamp, Temperature, Humidity, Pressure\n")
		fmt.Fprintf(w, "# Seconds, degree C, %% rel, hPa\n")
		for _, dp := range datapoints {
			fmt.Fprintf(w, "%d,%.2f,%.2f,%.2f\n", dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
		}


	} else {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad request\n"))
	}
}
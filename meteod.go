package main

import "fmt"
import "log"
import "net/http"
import "html/template"
import "strconv"
import "encoding/json"
import "io/ioutil"
import "time"
import "math"
import "github.com/BurntSushi/toml"
import "github.com/gorilla/mux"

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
	QueryLimit int64
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
	cf.Webserver.QueryLimit = 50000		// Be genereous by default
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

    r := mux.NewRouter()
    r.HandleFunc("/", dashboardOverview)
	r.Handle("/asset/{filename}", http.StripPrefix("/asset/", http.FileServer(http.Dir("www/asset"))))
	r.HandleFunc("/dashboard", dashboardOverview)
	r.HandleFunc("/dashboard/{id:[0-9]+}", dashboardStation)
	r.HandleFunc("/version", defaultHandler)
	r.HandleFunc("/stations", stationsHandler)
	r.HandleFunc("/station/{id:[0-9]+}", stationHandler)
	r.HandleFunc("/station/{id:[0-9]+}/current", stationReadingsHandler)
	r.HandleFunc("/station/{id:[0-9]+}/current.csv", stationReadingsHandler)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}/{month:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}/{month:[0-9]+}/{day:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/current", readingsHandler)
	r.HandleFunc("/latest", readingsHandler)
	r.HandleFunc("/readings", readingsHandler)
	r.HandleFunc("/query", queryHandler)
    http.Handle("/", r)
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



func v_str(values []string, emptyValue string) (string) {
	if len(values) == 0 {
		return emptyValue
	} else {
		return values[0]
	}
}

func v_i(values []string, emptyValue int) (int) {
	if len(values) == 0 { return emptyValue }
	// Take the first that works
	for _, v := range values {
		l, err := strconv.ParseInt(v, 10, 32)
		if err == nil { return int(l) }
	}
	return emptyValue
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

/* ==== Webserver =========================================================== */

func defaultHandler(w http.ResponseWriter, r *http.Request) {
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
	vars := mux.Vars(r)
	id, err := strconv.Atoi(vars["id"])
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad id\n"))
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
		limit := v_i(params["limit"], 1000)
		if limit < 0 || limit > 1000 { limit = 1000 }

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
			
			// XXX Reserved for future usage

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

		limit := v_l(values["limit"], cf.Webserver.QueryLimit)
		offset := v_l(values["offset"], 0)
		t_min := v_l(values["tmin"], 0)
		t_max := v_l(values["tmax"], time.Now().Unix())

		// Make values sane
		if limit < 0 || limit > cf.Webserver.QueryLimit { limit = cf.Webserver.QueryLimit }
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

type DashboardStation struct {
	Id int
	Name string
	Description string
	Location string
	T float32
	Hum float32
	P float32
}

type DashboardDataPoint struct {
	Timestamp   int64
	Time        string
	Temperature float32
	Humidity    float32
	Pressure    float32
}

func (s *DashboardStation) fromStation(station Station) {
	s.Id = station.Id
	s.Name = station.Name
	s.Description = station.Description
	s.Location = station.Location
}

/** Fetch latest points from database
    returns true if points has been fetched, false if not points are available */
func (s *DashboardStation) fetch() (bool, error) {
	dps, err := db.GetLastDataPoints(s.Id, 1)
	if err != nil { return false, err }
	if len(dps) > 0 {
		s.T = dps[0].Temperature
		s.Hum = dps[0].Humidity
		s.P = dps[0].Pressure
		return true, nil
	}
	return false, nil
}

func dashboardOverview(w http.ResponseWriter, r *http.Request) {	
	dbstations, err := db.GetStations()
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Server error"))
		panic(err)
	}
	
	stations := make([]DashboardStation, 0)
	for _, s := range dbstations {
		station := DashboardStation{Id:s.Id, Name: s.Name, Description:s.Description, Location: s.Location}
		_, err = station.fetch()
		
		stations = append(stations, station)
	}
	
	t, err := template.ParseFiles("www/dashboard.tmpl")
	err = t.Execute(w, stations)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
	}
	w.Write([]byte("</body>"))
}

func getVars(v map[string][]string) map[string]string {
	ret := make(map[string]string)
	for k, w := range v {
		if len(w) > 0 { ret[k] = w[0] }
	}
	return ret
}

/** Convert datapoints to dashboard datapoints and also shrink down to at most 1825 (365*5) datapoints */
func convertDatapoints(dps []DataPoint) ([]DashboardDataPoint) {
	nMax := 1825
	ret := make([]DashboardDataPoint, 0)

	// Shrink if necessary
	if len(dps) > nMax {
		dp0, dpN := dps[0], dps[len(dps)-1]
		deltaT := (dpN.Timestamp - dp0.Timestamp) / int64(nMax)
		j := 0	// Current index
		for i:=0;i<nMax;i++ {		// Slices
			t_min := dp0.Timestamp + deltaT * int64(i)
			t_max := t_min + deltaT
			
			ddp := DashboardDataPoint{Timestamp:t_min + deltaT/2}
			// Merge items
			counter := 0
			for j < len(dps) && dps[j].Timestamp <= t_max {
				ddp.Temperature += dps[j].Temperature
				ddp.Humidity += dps[j].Humidity
				ddp.Pressure += dps[j].Pressure
				j++
				counter++
			}
			if counter > 0 {
				ddp.Temperature /= float32(counter)
				ddp.Humidity /= float32(counter)
				ddp.Pressure /= float32(counter)
				ddp.Time = time.Unix(ddp.Timestamp, 0).Format("2006-01-02-15:04:05")
				ret = append(ret, ddp)
			}
		}

	} else {
		for _, dp := range dps {
			ddp := DashboardDataPoint{Timestamp:dp.Timestamp, Temperature:dp.Temperature, Humidity:dp.Humidity, Pressure:dp.Pressure}
			// Parse time
			t := time.Unix(dp.Timestamp, 0)
			ddp.Time = t.Format("2006-01-02-15:04:05")
			ret = append(ret, ddp)
		}
	}



	return ret
}

func dashboardStation(w http.ResponseWriter, r *http.Request) {	
	vars := mux.Vars(r)
	id, err := strconv.Atoi(vars["id"])
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Illegal id\n"))
		return
	}
	params := getVars(r.URL.Query())
	

	s, err := db.GetStation(id)
	if s.Id == 0 {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad id\n"))
		return
	}
	station := DashboardStation{}
	station.fromStation(s)
	station.fetch()		// Ignore errors when fetching latest datapoint

	t, err := template.ParseFiles("www/station.tmpl")
	err = t.Execute(w, station)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
		return
	}

	// Get data according to defined timespan
	timespan := params["timespan"]
	now := time.Now().Unix()
	t_max := now
	t_min := now - 60*60*24		// Default: last 24h
	if timespan == "week" {
		t_min = now - 60*60*24*7
	} else if timespan == "month" {
		t_min = now - 60*60*24*7*30
	} else if timespan == "year" {
		t_min = now - 60*60*24*7*365
	}
	// TODO: Limit and offset directive
	dps, err := db.QueryStation(station.Id, t_min, t_max, 50000, 0)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
		return
	}
	ddps := convertDatapoints(dps)
	t, err = template.ParseFiles("www/graph_t_hum.tmpl")
	err = t.Execute(w, ddps)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
		return
	}

	w.Write([]byte("</body>"))
}


func stationReadingsHandler(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	id, err := strconv.Atoi(vars["id"])
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Illegal id\n"))
		return
	}
	s, err := db.GetStation(id)
	if s.Id == 0 {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad id\n"))
		return
	}


	if r.Method == "GET" {
		dps, err := db.GetLastDataPoints(s.Id, 1)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error"))
			panic(err)
		}
		w.Write([]byte("# [Unixtimestamp],[deg C],[% rel],[hPa]\n"))
		if len(dps) > 0 {
			dp := dps[0]
			w.Write([]byte(fmt.Sprintf("%d,%3.2f,%3.2f,%3.2f\n", dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)))
		}
	} else {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad request\n"))
	}
}

func stationQueryCsv(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	id, err := strconv.Atoi(vars["id"])
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Illegal id\n"))
		return
	}
	year, err := strconv.Atoi(vars["year"])
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad year\n"))
		return
	}
	// Month and day are optional
	month, day := -1, -1
	if vars["month"] != "" {
		month, err = strconv.Atoi(vars["month"])
		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			w.Write([]byte("Bad month\n"))
			return
		}

		if vars["day"] != "" {
			day, err = strconv.Atoi(vars["day"])
			if err != nil {
				w.WriteHeader(http.StatusBadRequest)
				w.Write([]byte("Bad day\n"))
				return
			}
		}
	}

	s, err := db.GetStation(id)
	if s.Id == 0 {
		w.WriteHeader(http.StatusBadRequest)
		w.Write([]byte("Bad id\n"))
		return
	}
	
	
	var t_min time.Time
	var t_max time.Time
	if month < 0 && day < 0 {
		// func Date(year int, month Month, day, hour, min, sec, nsec int, loc *Location) Time
		t_min = time.Date(year, 0, 0, 0, 0, 0, 0, time.UTC)
		t_max = t_min.AddDate(1, 0, 0)
	} else if day < 0 {
		// Only month
		t_min = time.Date(year, 0, 0, 0, 0, 0, 0, time.UTC)
		t_min = t_min.AddDate(0, month, 0)
		t_max = t_min.AddDate(0, 1, 0)
	} else {
		// Year, month and day
		t_min = time.Date(year, 0, 0, 0, 0, 0, 0, time.UTC)
		t_min = t_min.AddDate(0, month, day)
		t_max = t_min.AddDate(0, 0, 1)
	}
	// TODO: Limit and offset directive
	var limit int64
	var offset int64
	limit, offset = 10000, 0
	dps, err := db.QueryStation(s.Id, t_min.Unix(), t_max.Unix(), limit, offset)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
		return
	}

	w.WriteHeader(http.StatusOK)
	w.Write([]byte(fmt.Sprintf("# Query station %s (id: %d) - %s\n", s.Name, s.Id, s.Description)))
	w.Write([]byte(fmt.Sprintf("# Location %s\n", s.Location)))
	w.Write([]byte(fmt.Sprintf("# Year: %d\n", year)))
	if month > 0 {w.Write([]byte(fmt.Sprintf("# Month: %d\n", month)))}
	if day > 0 {w.Write([]byte(fmt.Sprintf("# Day: %d\n", day)))}
	w.Write([]byte(fmt.Sprintf("# Datapoints: %d\n\n", len(dps))))
	w.Write([]byte("# Timestamp,Temperature,Humidity,Pressure\n"))
	w.Write([]byte("# [Unixtimestamp],[deg C],[% rel],[hPa]\n\n"))
	for _, dp := range dps {
		w.Write([]byte(fmt.Sprintf("%d,%3.2f,%3.2f,%3.2f\n", dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)))
	}
}

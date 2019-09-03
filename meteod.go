package main

import (
	"fmt"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"os"
	"strings"
)
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
	Database string  `toml:"Database"`
	Mqtt string `toml:MQTT`
	Webserver tomlWebserver `toml:"Webserver"`
	PushDelay int64
}

type tomlWebserver struct {
	Port     int
	Bindaddr string
	QueryLimit int64
}

var cf tomlConfig
var db Persistence




type jsonDataPoint struct {
	Token string
	Timestamp int64
	T float32
	Hum float32
	P float32
}

type mqttDataPoint struct {
	Timestamp int64 `json:"timestamp"`
	Station int `json:"node"`
	Name string `json:"name"`
	Temperature float32 `json:"t"`
	Humidity float32 `json:"hum"`
	Pressure float32 `json:"p"`
}

func (dp *mqttDataPoint) ToDataPoint() (DataPoint) {
	return DataPoint{ Timestamp: dp.Timestamp, Station:dp.Station, Temperature:dp.Temperature, Humidity:dp.Humidity, Pressure:dp.Pressure }
}

func (dp *mqttDataPoint) FromDataPoint(src DataPoint) {
	dp.Timestamp = src.Timestamp
	dp.Station = src.Station
	dp.Temperature = src.Temperature
	dp.Humidity = src.Humidity
	dp.Pressure = src.Pressure
}

var mqttClient mqtt.Client			// If using MQTT, this is the MQTT client
var mqttDp mqttDataPoint			// Ignore this datapoint, as we are pushing it right now

// remote: [username[:password]@]hostname[:port]
func attachMqtt(remote string) {
	username := ""
	password := ""
	port := 1883
	hostname := remote
	topic := "meteo/#"

	if strings.Contains(hostname, "@") {
		i := strings.Index(hostname, "@")
		username = hostname[:i]
		hostname = hostname[i+1:]
		if strings.Contains(username, ":") {
			i = strings.Index(hostname, ":")
			password = username[i+1:]
			username = username[:i]
		}
	}
	if strings.Contains(hostname, ":") {
		i := strings.Index(hostname, ":")
		port, _ = strconv.Atoi(hostname[i+1:])		// Swallow error here
		hostname = hostname[:i]
	}

	opts := mqtt.NewClientOptions()
	remote = fmt.Sprintf("tcp://%s:%d", hostname, port)
	opts.AddBroker(remote)
	if username != "" { opts.SetUsername(username) }
	if password != "" { opts.SetPassword(password) }

	log.Println("Attaching MQTT: " + remote + " ... ")
	mqttClient = mqtt.NewClient(opts)
	token := mqttClient.Connect()
	for !token.WaitTimeout(5 * time.Second) {}
	if err := token.Error(); err != nil {
		fmt.Fprintf(os.Stderr, "Error connecting MQTT: %s", err)
		return
	}
	token = mqttClient.Subscribe(topic, 0, mqttReceive)
	token.Wait()
	if err := token.Error(); err != nil {
		fmt.Fprintf(os.Stderr, "Error subscribing to MQTT: %s", err)
		return
	}
	log.Printf("MQTT: " + remote + " attached (listening to topic '%s')\n", topic)
}

func mqttJsonParse(nodeId int, message string) (mqttDataPoint, error) {
	dp := mqttDataPoint{Station:nodeId}

	// Parse json packets
	var dat map[string]interface{}
	if err := json.Unmarshal([]byte(message), &dat); err != nil {
		return dp, err
	}
	// Get contents from json message
	/*		// Consider adding a type for other nodes?
	if dat["_type"].(string) != "location" {
		return loc, errors.New("Illegal json type")
	} else {
	*/
	if dat["timestamp"] != nil { dp.Timestamp = int64(dat["timestamp"].(float64)) }
	if dat["t"] != nil { dp.Temperature = float32(dat["t"].(float64)) }
	if dat["name"] != nil { dp.Name = dat["name"].(string) }
	if dat["hum"] != nil { dp.Humidity = float32(dat["hum"].(float64)) }
	if dat["p"] != nil { dp.Pressure = float32(dat["p"].(float64)) }

	return dp, nil
}

func mqttReceive(client mqtt.Client, msg mqtt.Message) {
	topic := msg.Topic()
	payload := string(msg.Payload())
	if topic == "" || payload == "" { return }

	// Get station id from topic
	i := strings.Index(topic, "/")
	if i < 0 {
		fmt.Printf("MQTT: Topic format mismatch: %s\n", topic)
		return
	}
	node_id := topic[i+1:]
	node_type := ""
	i = strings.Index(node_id, "/")
	if i<0 {
		node_type = ""
	} else {
		node_type = node_id[i+1:]
		node_id = node_id[:i]
	}
	nodeId, err := strconv.Atoi(node_id)
	if err != nil {
		log.Printf("MQTT: Meteo node format mismatch: %s\n", topic)
		return
	}

	if node_type == "" || node_type == "meteo" {		// Default packet
		dp, err := mqttJsonParse(nodeId, payload)
		if err != nil {
			log.Println("MQTT illegal packet: ", err)
			return
		}
		if dp == mqttDp {
			return
		} // Ignore datapoint, as we have just published exactly this one

		// Assign timestamp, if not given by message
		if dp.Timestamp == 0 {
			dp.Timestamp = time.Now().Unix()
		}

		// Check if station exists, otherwise create it
		exists, err := db.ExistsStation(dp.Station)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Database error (ExistsStation): %s\n", err)
			return
		}
		if !exists {
			station := Station{Id: dp.Station, Name: dp.Name}
			err = db.InsertStation(&station)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Database error (InsertStation): %s\n", err)
				return
			}
			log.Printf("Created station \"%s\" [%d]\n", dp.Name, dp.Station)
		}

		// Convert from JSON datapoint to DataPoint structure
		if !received(dp.ToDataPoint()) {
			log.Println("Error handling received datapoint")
		}
	}
}

func mqttPublish(dp DataPoint) {
	if mqttClient != nil {
		// Create JSON packet
		station, err := db.GetStation(dp.Station)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error getting station: %s\n", err)
			return;
		}

		jsonDp := mqttDataPoint{}
		jsonDp.FromDataPoint(dp)
		jsonDp.Name = station.Name
		converted, err := json.Marshal(jsonDp)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error marshalling json for MQTT: %s\n", err)
			return
		}

		topic := fmt.Sprintf("meteo/%d", dp.Station)
		payload := string(converted)
		mqttDp.FromDataPoint(dp)
		mqttDp.Name = station.Name
		token := mqttClient.Publish(topic, 0, false, payload)
		token.Wait()
		if err = token.Error(); err != nil {
			fmt.Fprintf(os.Stderr, "Error publishing to MQTT: %s\n", err)
		}

	}
}




func main() {
	var err error 

	// Default config values
	cf.PushDelay = 60		// 60 Seconds
	cf.Database = "meteod.db"
	cf.Webserver.Port = 8802
	cf.Webserver.QueryLimit = 50000		// Be genereous by default
	// Read config
	toml.DecodeFile("/etc/meteod.toml", &cf)
	toml.DecodeFile("meteod.toml", &cf)

	// Parse program arguments
	// Note: I am doing this by hand, because I don't wanted to have another dependency
	// This might change in the future, when enough people have convinced me, this is a bad idea
	for i:=1;i<len(os.Args);i++ {
		arg := os.Args[i]
		if len(arg) == 0 { continue }
		if arg[0] == '-' {

			if arg == "-h" || arg == "--help" {
				fmt.Println("meteod - meteo server daemon")
				fmt.Printf("Usage: %s [OPTIONS]\n\n", os.Args[0])
				fmt.Println("OPTIONS")
				fmt.Println("  -h, --help            Print this help message")
				fmt.Println("  -p, --port PORT       Set webserver port")
				fmt.Println("  -d, --db FILE         Set database file")
				fmt.Println("  -c, --config FILE     Load config file FILE")
				fmt.Println("                        This is an additional config file to the default ones")
				fmt.Println("  -m, --mqtt MQTT       Connect to the given MQTT server.")
				fmt.Println("                        Format: [user@]remote[:port]")
				os.Exit(0)
			} else if arg == "-p" || arg == "--port" {
				i+=1
				if i >= len(os.Args) { panic("Missing argument: port")}
				cf.Webserver.Port, err = strconv.Atoi(os.Args[i])
				if err != nil {panic("Illegal port given")}
			} else if arg == "-d" || arg == "--db" {
				i+=1
				if i >= len(os.Args) { panic("Missing argument: db file")}
				cf.Database = os.Args[i]
			} else if arg == "-c" || arg == "--config" {
				i+=1
				if i >= len(os.Args) { panic("Missing argument: config file")}
				toml.DecodeFile(os.Args[i], &cf)
			} else if arg == "-m" || arg == "--mqtt" {
				i+=1
				if i >= len(os.Args) { panic("Missing argument: mqtt remote host")}
				cf.Mqtt = os.Args[i]
			} else {
				fmt.Printf("Illegal argument: %s\n", arg)
				os.Exit(1)
			}

		} else {
			fmt.Printf("Illegal argument: %s\n", arg)
			os.Exit(1)
		}
	}



	// Connect database
	db, err = OpenDb(cf.Database)
	if err != nil {
		panic(err)
	}
	if err := db.Prepare(); err != nil {
		panic(err)
	}
	log.Println("Database loaded")

	// Attach mqtt, if configured
	if cf.Mqtt != "" { go attachMqtt(cf.Mqtt) }

	// Setup webserver
	addr := cf.Webserver.Bindaddr + ":" + strconv.Itoa(cf.Webserver.Port)
	log.Println("Serving http://" + addr + "")

    r := mux.NewRouter()
    r.HandleFunc("/", dashboardOverview)
	r.Handle("/asset/{filename}", http.StripPrefix("/asset/", http.FileServer(http.Dir("www/asset"))))
	r.HandleFunc("/dashboard", dashboardOverview)
	r.HandleFunc("/dashboard/{id:[0-9]+}", dashboardStation)
	r.HandleFunc("/api.html", func(w http.ResponseWriter, r *http.Request) {
    	http.ServeFile(w, r, "www/api.html")
	})
	r.HandleFunc("/health", HealthCheckHandler)
	r.HandleFunc("/version", defaultHandler)
	r.HandleFunc("/stations", stationsHandler)
	r.HandleFunc("/station/{id:[0-9]+}", stationHandler)
	r.HandleFunc("/station/{id:[0-9]+}/current", stationReadingsHandler)
	r.HandleFunc("/station/{id:[0-9]+}/current.csv", stationReadingsHandler)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}/{month:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/station/{id:[0-9]+}/{year:[0-9]+}/{month:[0-9]+}/{day:[0-9]+}.csv", stationQueryCsv)
	r.HandleFunc("/current", readingsHandler)
	r.HandleFunc("/current.csv", readingsHandlerCsv)
	r.HandleFunc("/latest", readingsHandler)
	r.HandleFunc("/readings", readingsHandler)
	r.HandleFunc("/query", queryHandler)
    http.Handle("/", r)
	log.Fatal(http.ListenAndServe(addr, nil))
}

func doDatapointPush(dp DataPoint) (bool, error) {
	datapoints, err := db.GetLastDataPoints(dp.Station, 1)
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
		if math.Abs((float64)(last.Temperature - dp.Temperature)) > 0.5 { return true, nil }
		if math.Abs((float64)(last.Pressure - dp.Pressure)) > 100 { return true, nil }
		if math.Abs((float64)(last.Humidity - dp.Humidity)) > 2 { return true, nil }
	}
	return false, nil
}

func isPlausible(dp DataPoint) bool {
	if dp.Temperature < -100 || dp.Temperature > 1000 { return false }
	if dp.Humidity < 0 || dp.Humidity > 100 { return false }
	p_mbar := dp.Pressure / 100.0
	if p_mbar != 0 {
		if p_mbar < 10 {
			return false
		}
		if p_mbar > 2000 {
			return false
		} // We are not a submarine!
	}
	return true
}

/* Function callback when received a datapoint. Return true if the datapoint is accepted
 * Accepted datapoint doesn't mean automatically pushed datapoint.
 */
func receivedDpToken(dp jsonDataPoint) bool {
	if dp.Token == "" {
		return false
	} // No token - Ignore

	// Set timestamp if not set by sender
	if dp.Timestamp == 0 {
		dp.Timestamp = time.Now().Unix()
	}

	token, err := db.GetToken(dp.Token)
	if err != nil {
		panic(err)
	}
	station := token.Station
	if station <= 0 {
		return false
	} // Denied

	exists, err := db.ExistsStation(station)
	if err != nil {
		panic(err)
	}
	if !exists {
		//fmt.Printf("Station doesn't exists: %d\n", station)
		return false
	}

	datapoint := DataPoint{
		Timestamp:   dp.Timestamp,
		Station:     token.Station,
		Temperature: dp.T,
		Humidity:    dp.Hum,
		Pressure:    dp.P,
	}

	ret := received(datapoint)
	mqttPublish(datapoint)
	return ret
}

func received(dp DataPoint) bool {
	// Repair pressure unit, that could be in mbar but is expected in hPa
	if dp.Pressure < 10000 { dp.Pressure *= 100 }

	// Check ranges for plausability
	if !isPlausible(dp) {
		fmt.Printf("UNPLAUSIBLE: %d at %d, t = %f, hum = %f, p = %f\n", dp.Station, dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
		return true
	}

	push, err := doDatapointPush(dp)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Database error when determining if datapoint is recent: ", err)
		return false
	}



	if push {
		db_dp := DataPoint{Timestamp: dp.Timestamp, Station: dp.Station, Temperature: dp.Temperature, Humidity: dp.Humidity, Pressure: dp.Pressure}
		err := db.InsertDataPoint(db_dp)
		if err != nil { panic(err) }
		log.Printf("PUSH: [%d] at %d, t = %f, hum = %f, p = %f\n", dp.Station, dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
	} else {
		log.Printf("RECV: [%d] at %d, t = %f, hum = %f, p = %f\n", dp.Station, dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
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

func HealthCheckHandler(w http.ResponseWriter, r *http.Request) {
    // A very simple health check.
    w.Header().Set("Content-Type", "application/json")
    w.WriteHeader(http.StatusOK)
    w.Write([]byte(`{"alive": true}`))
}


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

func readingsHandlerCsv(w http.ResponseWriter, r *http.Request) {	
	if r.Method == "GET" {
		stations, err := db.GetStations()
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Server error"))
			panic(err)
		} else {

			w.Header().Set("Content-Disposition", "attachment; filename=\"current.csv\"")
			w.Header().Set("Content-Type", "text/csv")
			w.WriteHeader(http.StatusOK)
			fmt.Fprintf(w, "# Station, Timestamp, Temperature, Humidity, Pressure\n")
			for _, station := range stations {
				dps, err := db.GetLastDataPoints(station.Id, 1)
				if err != nil {
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
				msg := fmt.Sprintf("Illegal json received: %s\n", err)
				w.Write([]byte(msg))
				return
			}
			ok := receivedDpToken(dp)
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
	dps, err := db.QueryStation(station.Id, t_min, t_max, 100000, 0)
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
		filename := fmt.Sprintf("current_%02d.csv", s.Id)
		w.Header().Set("Content-Disposition", "attachment; filename=" + filename)
		w.Header().Set("Content-Type", "text/csv")
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
	limit, offset = 0, 0
	dps, err := db.QueryStation(s.Id, t_min.Unix(), t_max.Unix(), limit, offset)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
		return
	}

	filename := fmt.Sprintf("station_%02d-%04d", s.Id, year)
	if month > 0 { filename += fmt.Sprintf("-%02d", month) }
	if day > 0 { filename += fmt.Sprintf("-%02d", day) }
	filename += ".csv"
	w.Header().Set("Content-Disposition", "attachment; filename=" + filename)
	w.Header().Set("Content-Type", "text/csv")
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

package main

import (
	"fmt"
	"log"
	"os"
	"time"
	"sync"
	"strconv"
	"github.com/eclipse/paho.mqtt.golang"
	"github.com/buger/jsonparser"
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
	"net/http"
)


var db *sql.DB

type station struct {
	id int
	name string
	t float32
	p float32
	hum float32
	alive bool
}

type weatherpoint struct {
	timestamp int64
	t float32
	p float32
	hum float32
}

func (s *station) str() string {
	return fmt.Sprintf("%s (id: %d) t = %f, p = %f, hum = %f", s.name,s.id,s.t,s.p,s.hum)
}

func (s *station) update(t float32, p float32, hum float32) {
	const ALPHA = 0.75
	s.t = ALPHA*s.t + (1.0-ALPHA)*t
	s.p = ALPHA*s.p + (1.0-ALPHA)*p
	s.hum = ALPHA*s.hum + (1.0-ALPHA)*hum
	s.alive = true
}

var stations map[int]station
var mutex = &sync.Mutex{}

func onMessageReceived(client mqtt.Client, message mqtt.Message) {
	data := message.Payload()
	
	//fmt.Printf("%s : %s\n", message.Topic(), data)
	
	// Extract data, return if not valid
	id, err := jsonparser.GetInt(data, "id")
	if err != nil { fmt.Println("Error: id" ); return; }
	name, err := jsonparser.GetString(data, "name")
	if err != nil { fmt.Println("Error: name" ); return; }
	t, err := jsonparser.GetFloat(data, "t")
	if err != nil { fmt.Println("Error: t" ); return; }
	p, err := jsonparser.GetFloat(data, "p")
	if err != nil { fmt.Println("Error: p" ); return; }
	hum, err := jsonparser.GetFloat(data, "hum")
	if err != nil { fmt.Println("Error: hum" ); return; }
	
	
	s := station{id:int(id), name:name, t:float32(t), p:float32(p), hum:float32(hum), alive:true};
	//fmt.Println("RECV  " + s.str())
	
	mutex.Lock()
	
	_, present := stations[int(id)]
	if present {
		s1 := stations[int(id)]
		s1.update(float32(t),float32(p),float32(hum))
		s = s1
	}
	stations[int(id)] = s
	mutex.Unlock()
	
	fmt.Println("RECV  " + s.str())
}

func dbSetup() {
	sqlStmt := "CREATE TABLE IF NOT EXISTS `stations` (`id` INT PRIMARY KEY, `name` TEXT);"
	_, err := db.Exec(sqlStmt)
	if err != nil {
		log.Printf("%q: %s\n", err, sqlStmt)
		log.Fatal(err)
	}
}

func get_timestamp() int64 {
	now := time.Now()
	timestamp := now.Unix()
	return timestamp
}

func pushStation(s station) {
	sql := "CREATE TABLE IF NOT EXISTS `station_" + strconv.Itoa(s.id) + "` (`timestamp` INT, `t` FLOAT, `p` FLOAT, `hum` FLOAT);"
	_,err := db.Exec(sql)
	if err != nil {
		log.Fatal("Create table failed; ", err)
	}
	
	tx, err := db.Begin()
	if err != nil {
		log.Fatal(err)
	}
	
	sql = "REPLACE INTO `stations` (`id`,`name`) VALUES (?,?)"
	stmt, err := tx.Prepare(sql)
	if err != nil {
		log.Fatal("Prepare statement (1) failed; ", err)
	}
	_,err = stmt.Exec(s.id,s.name)
	if err != nil {
		log.Fatal("Insert failed; ", err)
	}
	stmt.Close()
	
	sql = "REPLACE INTO `station_" + strconv.Itoa(s.id) + "` (`timestamp`, `t`, `p`, `hum`) VALUES (?,?,?,?);"
	stmt, err = tx.Prepare(sql)
	if err != nil {
		log.Fatal("Prepare statement (2) failed; ", err)
	}
	_,err = stmt.Exec(get_timestamp(),s.t,s.p,s.hum)
	if err != nil {
		log.Fatal("Insert failed; ", err)
	}
	stmt.Close()
	
	tx.Commit()
	//fmt.Println("WROTE      ", s.str())
}

func pushDB() {
	//fmt.Println("Database thread started")
	for {
		time.Sleep(5 * time.Minute)			// Every 5 minutes
		
		
		// Get stations and write to database
		mutex.Lock()
		//fmt.Println("Writing to database ... ")
		var counter int = 0
		for id, s := range stations { 
			if s.alive {
				pushStation(s)
				s.alive = false
				counter++
			} else {
				// Remove from map
				delete(stations, id)
				continue
			}
			stations[id] = s
		}
		mutex.Unlock()
		
		if counter > 0 {
			if counter == 1 {
				fmt.Printf("Wrote 1 entry to database\n")
			} else {
				fmt.Printf("Wrote %d entries to database\n", counter)
			}
		}
	}	
}

func db_stations() []station {
	rows, err := db.Query("SELECT `id`,`name` FROM `stations` ORDER BY `id` ASC")
	if err != nil {
		log.Fatal(err)
	}
	
	var (
		id int
		name string
	)
	
	ret := make([]station, 0)
	for rows.Next() {
		rows.Scan(&id, &name)
		
		s := station{id:id, name:name};
		ret = append(ret, s)
	}
	
	return ret
}

func db_station(id int, min_t int, max_t int, limit int) ([]weatherpoint,error) {
	ret := make([]weatherpoint, 0)
	
	if min_t < 0 { min_t = 0 }
	if max_t <= 0 { max_t = int(get_timestamp()) }
	
	sql := "SELECT `timestamp`,`t`,`p`,`hum` FROM `station_" + strconv.Itoa(id) + "` WHERE `timestamp` >= '" + strconv.Itoa(min_t) + "' AND `timestamp` <= '" + strconv.Itoa(max_t) + "' ORDER BY `timestamp` ASC LIMIT " + strconv.Itoa(limit)
	fmt.Println(sql)
	rows, err := db.Query(sql)
	if err != nil {
		return ret, err
	}
	
	var (
		timestamp int64
		t float32
		p float32
		hum float32
	)
	for rows.Next() {
		rows.Scan(&timestamp, &t,&p,&hum)
		
		s := weatherpoint{timestamp:timestamp,t:t,p:p,hum:hum};
		ret = append(ret, s)
	}
	
	return ret,nil
}


//func www_handler_index(w http.ResponseWriter, r *http.Request) {
func www_handler_index(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "<html><head><meta http-equiv=\"refresh\" content=\"30\"><title>meteo</title></head>\n<body>")
	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")
	
	fmt.Fprintf(w, "<h2>Stations</h2>\n")
	// List all stations
	mutex.Lock()
	if len(stations) == 0 {
		fmt.Fprintf(w, "<p>Currently no stations active</p>\n")
	} else {
		fmt.Fprintf(w, "<table border=\"1\">\n")
		fmt.Fprintf(w, "<tr><td><b>Station</b></td><td><b>Temperature [deg C]</b></td><td><b>Pressure [hPa]</b></td><td><b>Humditiy [rel]</b></td></tr>\n")
		for id, s := range stations { 
			fmt.Fprintf(w, "<tr><td><a href=\"station?id=%d\">%s</a></td><td>%f</td><td>%f</td><td>%f</td></tr>\n", id, s.name, s.t, s.p, s.hum)
		}
		fmt.Fprintf(w, "</table>")
		fmt.Fprintf(w, "<p>Active stations: <b>%d</b></p>\n", len(stations))
	}
	mutex.Unlock()
	fmt.Fprintf(w, "<hr/><p><a href=\"https://github.com/grisu48/meteo\" target=\"_BLANK\">meteo</a>, 2018 phoenix</p>")
	fmt.Fprintf(w, "</body></html>")
}

//func www_handler_index(w http.ResponseWriter, r *http.Request) {
func www_handler_lightnings(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "<html><head><meta http-equiv=\"refresh\" content=\"10\"><title>meteo</title></head>\n<body>")
	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")
	
	fmt.Fprintf(w, "<h2>Lightnings</h2>\n")
	fmt.Fprintf(w, "<p><i>This feature is under construction</i></p>\n")
	
}

func www_handler_station(w http.ResponseWriter, r *http.Request) {
	
	s_id := r.URL.Query().Get("id")
	if len(s_id) == 0 {
		w.WriteHeader(http.StatusInternalServerError)
	    w.Write([]byte("500 - No station id defined"))
		return;
	}
	id,err := strconv.Atoi(s_id)
	
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
	    w.Write([]byte("500 - Illegal station identifier"))
		return
	}
	
	mutex.Lock()
	station, exists := stations[id]
	mutex.Unlock()
	
	if !exists {
		w.WriteHeader(http.StatusInternalServerError)
	    w.Write([]byte("500 - No such station active"))
		return
	}
	
	fmt.Fprintf(w, "<html><head><meta http-equiv=\"refresh\" content=\"10\"><title>meteo</title></head>\n<body>")
	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")
	fmt.Fprintf(w, "<h2>Station " + station.name + "</h2>\n")
	
	fmt.Fprintf(w, "<p>Current readings</p>\n")
	fmt.Fprintf(w, "<table border=\"1\">\n")
	fmt.Fprintf(w, "<tr><td><b>Station</b></td><td><b>Temperature [deg C]</b></td><td><b>Pressure [hPa]</b></td><td><b>Humditiy [rel]</b></td></tr>\n")
	fmt.Fprintf(w, "<tr><td><a href=\"station?id=%d\">%s</a></td><td>%f</td><td>%f</td><td>%f</td></tr>\n", id, station.name, station.t, station.p, station.hum)
	fmt.Fprintf(w, "</table>")
}

func www_handler_stations_csv(w http.ResponseWriter, r *http.Request) {
	st := db_stations()
	for _, v := range st {
		fmt.Fprintf(w, "%d,%s\n", v.id, v.name)
	}
}


func www_handler_station_csv(w http.ResponseWriter, r *http.Request) {
	id,err := strconv.Atoi(r.URL.Query().Get("id"))
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
	    w.Write([]byte("500 - Illegal station identifier"))
		return
	}
	
	t_min,err := strconv.Atoi(r.URL.Query().Get("tmin"))
	if err != nil { t_min = 0 }
	t_max,err := strconv.Atoi(r.URL.Query().Get("tmax"))
	if err != nil { t_max = 0 }
	
	values,err := db_station(id, t_min, t_max, 1000)		// Limit 1000 by default
	
	if err != nil {
		fmt.Fprintln(os.Stderr, " (!!) SQL request error:", err)
		w.WriteHeader(http.StatusInternalServerError)
	    w.Write([]byte("500 - Server error"))
		return
	}
	
	fmt.Fprintf(w, "# Timestamp, Temperature, Pressure, Humidity\n")
	fmt.Fprintf(w, "s, deg C, hPa, rel\n")
	for _,v := range values {
		fmt.Fprintf(w, "%d,%f,%f,%f\n", v.timestamp, v.t, v.p, v.hum)
	}
}


func main() {
	stations = make(map[int]station)
	var err error
	db, err = sql.Open("sqlite3", "meteod.db")
	if err != nil {
		log.Fatal(err)
	}
	dbSetup()
	defer db.Close()
	
	var host string = "127.0.0.1"
	var port = 1883
	//var wwwport = 8559
	
	if len(os.Args) > 1 {
		host = os.Args[1]
	}
	
	
	//c := make(chan os.Signal, 1)
	//signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	
	//mqtt.DEBUG = log.New(os.Stderr, "", 0)			// Disable logging
	mqtt.ERROR = log.New(os.Stderr, "", 0)
	var remote string = "tcp://" + host + ":" + strconv.Itoa(port)
	opts := mqtt.NewClientOptions().AddBroker(remote).SetClientID("meteod")
	opts.SetKeepAlive(30 * time.Second)
	opts.SetPingTimeout(5 * time.Second)
	opts.OnConnect = func(c mqtt.Client) {	
		fmt.Println("MQTT connected: " + remote)
		if token := c.Subscribe("meteo/#", 0, onMessageReceived); token.Wait() && token.Error() != nil {
			fmt.Println(token.Error())
			os.Exit(1)
		}
	}

	fmt.Println("Setting up mosquitto ...")
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	
	go pushDB()		// Fork push DB thread
	
	// Setup webserver
	//fmt.Println("Firing up webserver ... ")
	http.HandleFunc("/", www_handler_index)
	http.HandleFunc("/stations", www_handler_index)
	http.HandleFunc("/stations.csv", www_handler_stations_csv)
	http.HandleFunc("/station.csv", www_handler_station_csv)
	http.HandleFunc("/station", www_handler_station)
	http.HandleFunc("/lightnings", www_handler_lightnings)
	fmt.Println("Webserver started: http://127.0.0.1:8558")
	http.ListenAndServe(":8558", nil)
	
	//<-c
	fmt.Println("Bye")
}

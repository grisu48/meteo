package main

import (
	"fmt"
	"log"
	"os"
	"math"
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
	return fmt.Sprintf("%s (id: %d) t = %.2f, p = %.2f, hum = %.2f", s.name,s.id,s.t,s.p/100,s.hum)
}

func sqr(x float32) float32 {
	return (x*x);
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
	id, err := jsonparser.GetInt(data, "node")
	if err != nil { fmt.Println("Receive error: json attribute 'id' missing" ); return; }
	name, err := jsonparser.GetString(data, "name")
	if err != nil { fmt.Println("Receive error: json attribute 'name' missing" ); return; }
	t, err := jsonparser.GetFloat(data, "t")
	if err != nil { t = 0.0 }
	p, err := jsonparser.GetFloat(data, "p")
	if err != nil { p = 0.0 }
	hum, err := jsonparser.GetFloat(data, "hum")
	if err != nil { hum = 0.0 }


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
	//fmt.Println(sql)
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


func www_handler_index(w http.ResponseWriter, r *http.Request) {
	// Head
	fmt.Fprintf(w, "<!DOCTYPE html>\n")
	fmt.Fprintf(w, "<html>\n<head>")
	fmt.Fprintf(w, "<meta http-equiv=\"refresh\" content=\"30\">")
	fmt.Fprintf(w, "<title>meteo</title>")
	fmt.Fprintf(w, "<link rel=\"stylesheet\" type=\"text/css\" href=\"meteo.css\">")
	fmt.Fprintf(w, "</head>\n<body>")

	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")

	all_stations := make(map[int]station)
	for id, s := range db_stations() {
		all_stations[id] = s
	}

	fmt.Fprintf(w, "<h2>Stations</h2>\n")
	// List all active stations
	mutex.Lock()
	fmt.Fprintf(w, "<table border=\"1\">\n")
	fmt.Fprintf(w, "<tr><td><b>Station</b></td><td><b>Temperature [deg C]</b></td><td><b>Pressure [hPa]</b></td><td><b>Humditiy [rel]</b></td></tr>\n")
	if len(all_stations) == 0 && len(stations) == 0 {
		fmt.Fprintf(w, "</table>")
		fmt.Fprintf(w, "<p>No stations known</p>\n")
	} else {
		c_offline, c_volatile := 0, 0

		for id, s := range all_stations {
			id = s.id
			cur, exists := stations[id];
			if exists {
				fmt.Fprintf(w, "<tr><td><a href=\"station?id=%d\">%s</a></td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", id, cur.name, cur.t, cur.p/100, cur.hum)
			} else {
				fmt.Fprintf(w, "<tr><td><del><a href=\"station?id=%d\">%s</a></del></td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", id, s.name, s.t, s.p/100, s.hum)
				c_offline += 1
			}
		}
		// List as well all stations, that are active, but not yet in the database
		for id, s := range stations {
			_, exists := all_stations[id];
			if !exists {
				fmt.Fprintf(w, "<tr><td><i><a href=\"station?id=%d\">%s</a></i></td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", id, s.name, s.t, s.p/100, s.hum)
				c_volatile += 1
			}
		}
		fmt.Fprintf(w, "</table>")
		fmt.Fprintf(w, "<p>Active stations: <b>%d</b>\n", len(stations))
		if c_offline > 0 {fmt.Fprintf(w, "<br/><del>Offline</del> stations: <b>%d</b>\n", c_offline) }
		if c_volatile > 0 {fmt.Fprintf(w, "<br/><i>Volatile</i> stations: <b>%d</b> (i.e. Not yet pushed to db)", c_volatile) }
		fmt.Fprintf(w, "</p>")
	}
	mutex.Unlock()
	fmt.Fprintf(w, "<hr/><p><a href=\"https://github.com/grisu48/meteo\" target=\"_BLANK\">meteo</a>, 2018 phoenix</p>")
	fmt.Fprintf(w, "</body></html>")
}

//func www_handler_index(w http.ResponseWriter, r *http.Request) {
func www_handler_lightnings(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "<!DOCTYPE html>\n")
	fmt.Fprintf(w, "<html>\n<head>")
	//fmt.Fprintf(w, "<meta http-equiv=\"refresh\" content=\"10\">")
	fmt.Fprintf(w, "<title>meteo</title>")
	fmt.Fprintf(w, "<link rel=\"stylesheet\" type=\"text/css\" href=\"meteo.css\">")
	fmt.Fprintf(w, "</head>\n<body>")

	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")

	fmt.Fprintf(w, "<h2>Lightnings</h2>\n")
	fmt.Fprintf(w, "<p><i>This feature is under construction</i></p>\n")

}

func bin_data(data []weatherpoint, bins int) ([]weatherpoint) {
	if len(data) <= bins {
		return data
	} else {
		// TODO: This is untested
		ret := make([]weatherpoint, bins, bins)
		avg_bins := len(data)/bins
		for i := 0; i < bins; i++ {
			var p weatherpoint
			for j := 0; j < avg_bins; j++ {
				p.timestamp += data[i*avg_bins+j].timestamp
				p.t += data[i*avg_bins+j].t
				p.hum += data[i*avg_bins+j].hum
				p.p += data[i*avg_bins+j].p
			}
			fBins := float32(avg_bins)
			p.timestamp /= int64(avg_bins)
			p.t /= fBins
			p.hum /= fBins
			p.p /= fBins
			ret[i] = p
		}
		return ret
	}

}

func json_timestamps(data []weatherpoint) string {
	ret := "["

	first := true
	for _,v := range(data) {
		if first {
			first = false
		} else {
			ret += ","
		}
		timestamp := time.Unix(v.timestamp, 0)
		ret += "\"" + timestamp.String() + "\""
	}

	ret  += "]"
	return ret;
}
func json_temperature(data []weatherpoint) string {
	ret := "["
	first := true
	for _,v := range(data) {
		if first { first = false
		} else { ret += "," }
		ret += strconv.FormatFloat(float64(v.t), 'f', 4, 32)
	}
	ret  += "]"
	return ret;
}
func json_humidity(data []weatherpoint) string {
	ret := "["
	first := true
	for _,v := range(data) {
		if first { first = false
		} else { ret += "," }
		ret += strconv.FormatFloat(float64(v.hum), 'f', 4, 32)
	}
	ret  += "]"
	return ret;
}
func json_pressure(data []weatherpoint) string {
	ret := "["
	first := true
	for _,v := range(data) {
		if first { first = false
		} else { ret += "," }
		ret += strconv.FormatFloat(float64(v.p/100), 'f', 4, 32)
	}
	ret  += "]"
	return ret;
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
	// Head
	fmt.Fprintf(w, "<!DOCTYPE html>\n")
	fmt.Fprintf(w, "<html>\n<head>")
	//fmt.Fprintf(w, "<meta http-equiv=\"refresh\" content=\"10\">")
	fmt.Fprintf(w, "<title>meteo</title>")
	fmt.Fprintf(w, "<link rel=\"stylesheet\" type=\"text/css\" href=\"meteo.css\">")
	fmt.Fprintf(w, "</head>\n<body>")
	fmt.Fprintf(w, "<h1>meteo Web Portal</h1>\n")
	fmt.Fprintf(w, "<p><a href=\"stations\">[Stations]</a> <a href=\"lightnings\">[Lightnings]</a></p>\n")
	fmt.Fprintf(w, "<h2>meteo Station <b>" + station.name + "</b></h2>\n")

	fmt.Fprintf(w, "<h3>Current readings</h3>\n")
	fmt.Fprintf(w, "<table border=\"1\">\n")
	fmt.Fprintf(w, "<tr><td><b>Station</b></td><td><b>Temperature [deg C]</b></td><td><b>Pressure [hPa]</b></td><td><b>Humditiy [rel]</b></td></tr>\n")
	fmt.Fprintf(w, "<tr><td><a href=\"station?id=%d\">%s</a></td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", id, station.name, station.t, station.p/100, station.hum)
	fmt.Fprintf(w, "</table>")

	// What to get
	fmt.Fprintf(w, "<h3>Overview</h3>\n")
	href := "station?id=" + s_id
	fmt.Fprintf(w, "<p><a href=\"" + href + "&t=3h\">[Last 3h]</a> <a href=\"" + href + "&t=6h\">[Last 6h]</a> <a href=\"" + href + "&t=12h\">[Last 12h]</a> <a href=\"" + href + "&t=24h\">[Last 24h]</a></p>")
	now := get_timestamp()
	var t_min int64;
	s_time := r.URL.Query().Get("t")
	if s_time == "3h" {
		t_min = now - 3*60*60;
	} else if(s_time == "6h") {
		t_min = now - 6*60*60;
	} else if(s_time == "12h") {
		t_min = now - 12*60*60;
	} else if(s_time == "24h") {
		t_min = now - 24*60*60;
	} else {
		// Default value
		t_min = now - 3*60*60;
	}

	values,err := db_station(id, int(t_min), int(now), 1000)		// Limit 1000 by default
	if len(values) == 0 {
		fmt.Fprintf(w, "<p>No results</p>\n")
	} else {
		var avg, s_min, s_max, stdev weatherpoint

		first := true
		for _,v := range values {
			avg.t += v.t;
			avg.p += v.p;
			avg.hum += v.hum;
			if first {
				s_min.t = v.t
				s_min.p = v.p
				s_min.hum = v.hum
				s_max.t = v.t
				s_max.p = v.p
				s_max.hum = v.hum
			} else {
				if(v.t < s_min.t) { s_min.t = v.t }
				if(v.p < s_min.p) { s_min.p = v.p }
				if(v.hum < s_min.hum) { s_min.hum = v.hum }
				if(v.t > s_max.t) { s_max.t = v.t }
				if(v.p > s_max.p) { s_max.p = v.p }
				if(v.hum > s_max.hum) { s_max.hum = v.hum }
			}
		}
		fLen := float32(len(values))
		if len(values) > 0 {
			avg.t /= fLen
			avg.p /= fLen
			avg.hum /= fLen

			for _,v := range values {
				stdev.t += sqr(v.t-avg.t);
				stdev.p += sqr(v.p-avg.p);
				stdev.hum += sqr(v.hum-avg.hum);
			}
			stdev.t = float32(math.Sqrt(float64(stdev.t/sqr(fLen))))
			stdev.p /= float32(math.Sqrt(float64(stdev.p/sqr(fLen))))
			stdev.hum /= float32(math.Sqrt(float64(stdev.hum/sqr(fLen))))
		}

		fmt.Fprintf(w, "<p>Fetched %d data entries</p>\n", len(values))
		fmt.Fprintf(w, "<table border=\"1\">\n")
		fmt.Fprintf(w, "<tr><td></td><td>Average</td><td>Standart deviation</td><td>Minimum</td><td>Maximum</td></tr>\n")
		fmt.Fprintf(w, "<tr><td><b>Temperature [deg C]</b></td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", avg.t, stdev.t, s_min.t, s_max.t)
		fmt.Fprintf(w, "<tr><td><b>Pressure [hPa]</b></td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", avg.p/100, stdev.p/100, s_min.p/100, s_max.p/100)
		fmt.Fprintf(w, "<tr><td><b>Humidity [rel]</b></td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>\n", avg.hum, stdev.hum, s_min.hum, s_max.hum)
		fmt.Fprintf(w, "</table>")

		fmt.Fprintf(w, "<h3>Plots</h3>\n")
		fmt.Fprintf(w, "<script src=\"chart.js\"></script>\n")
		fmt.Fprintf(w, "<canvas id=\"plt_t\" width=\"undefined\" height=\"undefined\"></canvas>")
		fmt.Fprintf(w, "<canvas id=\"plt_h\" width=\"undefined\" height=\"undefined\"></canvas>")
		fmt.Fprintf(w, "<canvas id=\"plt_p\" width=\"undefined\" height=\"undefined\"></canvas>")
		fmt.Fprintf(w, "<script>\n")

		bins := 100		// Max bins (use 100 for now because why not)
		data := bin_data(values, bins)
		fmt.Printf("values: %d, bin_data: %d\n", len(values), len(data))

		// Declare datasets
		//fmt.Fprintf(w, "var datasets = { labels: " + json_timestamps(data) + ", datasets: [ { label:\"Temperature\", data:" + json_temperature(data) + ", borderColor: 'rgba(220, 0, 0, 0.8)'}, { label:\"Humidity\", data:" + json_humidity(data) + ", borderColor: 'rgba(0, 220, 0, 0.8)'}, { label:\"Pressure\", data:" + json_pressure(data) + ", borderColor: 'rgba(0, 0, 220, 0.8)'} ] };\n")
		fmt.Fprintf(w, "var data_t = { labels: " + json_timestamps(data) + ", datasets: [ { label:\"Temperature\", data:" + json_temperature(data) + ", borderColor: 'rgba(220, 0, 0, 0.8)'} ] };\n")
		fmt.Fprintf(w, "var data_h = { labels: " + json_timestamps(data) + ", datasets: [ { label:\"Humidity\", data:" + json_humidity(data) + ", borderColor: 'rgba(0, 220, 0, 0.8)'} ] };\n")
		fmt.Fprintf(w, "var data_p = { labels: " + json_timestamps(data) + ", datasets: [ { label:\"Pressure\", data:" + json_pressure(data) + ", borderColor: 'rgba(0, 0, 220, 0.8)'} ] };\n")
		//fmt.Fprintf(w, "var ctx = document.getElementById(\"plt_t\").getContext('2d');\n");
		//fmt.Fprintf(w, "var plot_t = new Chart(ctx, { \"type\":\"line\", \"data\": datasets } );\n")
		fmt.Fprintf(w, "var plot_t = new Chart(document.getElementById(\"plt_t\").getContext('2d'), { \"type\":\"line\", \"data\": data_t } );\n")
		fmt.Fprintf(w, "var plot_h = new Chart(document.getElementById(\"plt_h\").getContext('2d'), { \"type\":\"line\", \"data\": data_h } );\n")
		fmt.Fprintf(w, "var plot_p = new Chart(document.getElementById(\"plt_p\").getContext('2d'), { \"type\":\"line\", \"data\": data_p } );\n")

		fmt.Fprintf(w, "</script>\n")
	}
}

func www_handler_stations_csv(w http.ResponseWriter, r *http.Request) {
	st := db_stations()
	for _, v := range st {
		fmt.Fprintf(w, "%d,%s\n", v.id, v.name)
	}
}

func www_handler_chartjs(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "chart.js")
}

func www_handler_meteocss(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "meteo.css")
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
	limit,err := strconv.Atoi(r.URL.Query().Get("limit"))
	if err != nil || limit <= 0 { limit = 1000 }
	if limit > 1000 { limit = 1000 }

	values,err := db_station(id, t_min, t_max, limit)		// Limit 1000 by default

	if err != nil {
		fmt.Fprintln(os.Stderr, " (!!) SQL request error:", err)
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("500 - Server error"))
		return
	}

	fmt.Fprintf(w, "# Timestamp, Temperature, Pressure, Humidity\n")
	fmt.Fprintf(w, "s, deg C, hPa, rel\n")
	for _,v := range values {
		fmt.Fprintf(w, "%d,%.2f,%.2f,%.2f\n", v.timestamp, v.t, v.p/100, v.hum)
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
	opts := mqtt.NewClientOptions().AddBroker(remote)//.SetClientID("meteod")
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
	http.HandleFunc("/chart.js", www_handler_chartjs)
	http.HandleFunc("/meteo.css", www_handler_meteocss)
	http.HandleFunc("/stations", www_handler_index)
	http.Handle("/src/", http.FileServer(http.Dir("src/")))
	http.HandleFunc("/stations.csv", www_handler_stations_csv)
	http.HandleFunc("/station.csv", www_handler_station_csv)
	http.HandleFunc("/station", www_handler_station)
	http.HandleFunc("/lightnings", www_handler_lightnings)
	fmt.Println("Webserver started: http://127.0.0.1:8558")
	http.ListenAndServe(":8558", nil)

	//<-c
	fmt.Println("Bye")
}

package main

import (
	"fmt"
	"log"
	"os"
	"os/signal"
	"time"
	"syscall"
	"sync"
	"strconv"
	"github.com/eclipse/paho.mqtt.golang"
	"github.com/buger/jsonparser"
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
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
	
	now := time.Now()
	timestamp := now.Unix()
	
	sql = "REPLACE INTO `station_" + strconv.Itoa(s.id) + "` (`timestamp`, `t`, `p`, `hum`) VALUES (?,?,?,?);"
	stmt, err = tx.Prepare(sql)
	if err != nil {
		log.Fatal("Prepare statement (2) failed; ", err)
	}
	_,err = stmt.Exec(timestamp,s.t,s.p,s.hum)
	if err != nil {
		log.Fatal("Insert failed; ", err)
	}
	stmt.Close()
	
	tx.Commit()
	//fmt.Println("WROTE      ", s.str())
}

func pushDB() {
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
	
	if len(os.Args) > 1 {
		host = os.Args[1]
	}
	
	
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	
	//mqtt.DEBUG = log.New(os.Stderr, "", 0)			// Disable logging
	mqtt.ERROR = log.New(os.Stderr, "", 0)
	var remote string = "tcp://" + host + ":" + strconv.Itoa(port)
	opts := mqtt.NewClientOptions().AddBroker(remote).SetClientID("meteod")
	opts.SetKeepAlive(30 * time.Second)
	opts.SetPingTimeout(5 * time.Second)
	opts.OnConnect = func(c mqtt.Client) {	
		fmt.Println("Connected to " + remote)
		if token := c.Subscribe("meteo/#", 0, onMessageReceived); token.Wait() && token.Error() != nil {
			fmt.Println(token.Error())
			os.Exit(1)
		}
	}

	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	
	go pushDB()		// Fork push DB thread
	<-c
	fmt.Println("Bye")
}

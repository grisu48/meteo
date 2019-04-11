package main

import (
	"database/sql"
	_ "github.com/go-sql-driver/mysql"
	"strconv"
)

/* Basic datapoint */
type DataPoint struct {
	Timestamp   int64
	Station     int
	Temperature float32
	Humidity    float32
	Pressure    float32
}

/* Station */
type Station struct {
	Id          int
	Name        string
	Location    string
	Description string
}

type Persistence struct {
	con *sql.DB
}

/** Establish a mysql connection */
func ConnectDb(hostname string, username string, password string, database string, port int) (Persistence, error) {
	db := Persistence{}
	dSource := username + ":" + password + "@tcp(" + hostname + ":" + strconv.Itoa(port) + ")/" + database
	con, err := sql.Open("mysql", dSource)
	if err != nil {
		return db, err
	}
	err = con.Ping()
	if err != nil {
		return db, err
	}
	db.con = con

	return db, nil
}

/** Get the highest id from the given table */
func (db *Persistence) getHighestId(table string, id string) (int, error) {
	rows, err := db.con.Query("SELECT `" + id + "` FROM `" + table + "` ORDER BY `" + id + "` DESC LIMIT 1")
	if err != nil {
		return 0, err
	}
	defer rows.Close()
	if rows.Next() {
		var uid int
		rows.Scan(&uid)
		return uid, nil
	}
	return 0, nil
}

/* Prepare database, i.e. create tables ecc.
 */
func (db *Persistence) Prepare() error {
	_, err := db.con.Exec("CREATE TABLE IF NOT EXISTS `stations` (`id` INT PRIMARY KEY, `name` VARCHAR(64), `location` TEXT, `description` TEXT);")
	if err != nil {
		return err
	}

	return nil
}

/** Inserts the given station and assign the ID to the given station parameter */
func (db *Persistence) InsertStation(station *Station) error {
	id, err := db.getHighestId("stations", "id")
	if err != nil {
		return err
	}

	// Create table
	tablename := "station_" + strconv.Itoa(id)
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `" + tablename + "` (`timestamp` INT PRIMARY KEY, `temperature` FLOAT, `humidity` FLOAT, `pressure` FLOAT;")
	if err != nil {
		return err
	}

	stmt, err := db.con.Prepare("INSERT INTO `stations` (`id`, `name`, `location`, `description`) VALUES (?,?,?,?);")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(id, station.Name, station.Location, station.Description)
	if err != nil {
		return err
	}
	station.Id = id
	return nil
}

func (db *Persistence) GetStations() ([]Station, error) {
	stations := make([]Station, 0)

	rows, err := db.con.Query("SELECT `id`,`name`,`location`,`description` FROM `stations`")
	if err != nil {
		return stations, err
	}
	defer rows.Close()
	for rows.Next() {
		station := Station{}
		rows.Scan(&station.Id, &station.Name, &station.Location, &station.Description)
		stations = append(stations, station)
	}

	return stations, nil
}

func (db *Persistence) ExistsStation(id int) (bool, error) {
	stmt, err := db.con.Prepare("SELECT `id` FROM `stations` WHERE `id` = ? LIMIT 1")
	if err != nil {
		return false, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(id)
	if err != nil {
		return false, err
	}
	return rows.Next(), nil
}

func (db *Persistence) GetStation(id int) (Station, error) {
	station := Station{}

	stmt, err := db.con.Prepare("SELECT `id`,`name`,`location`,`description` FROM `stations` WHERE `id` = ? LIMIT 1")
	if err != nil {
		return station, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(id)
	if err != nil {
		return station, err
	}
	if rows.Next() {
		station := Station{}
		rows.Scan(&station.Id, &station.Name, &station.Location, &station.Description)
	}

	return station, nil
}

func (db *Persistence) GetLastDataPoints(station int, limit int) ([]DataPoint, error) {
	datapoints := make([]DataPoint, 0)
	tablename := "station_" + strconv.Itoa(station)
	stmt, err := db.con.Prepare("SELECT `timestamp`,`temperature`,`humidity`,`pressure` FROM `" + tablename + "` ORDER BY `timestamp` DESC LIMIT ?")
	if err != nil {
		return datapoints, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(limit)
	if err != nil {
		return datapoints, err
	}
	for rows.Next() {
		datapoint := DataPoint{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Temperature, &datapoint.Humidity, &datapoint.Pressure)
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

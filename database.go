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
	con.SetMaxOpenConns(5)
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
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `tokens` (`token` VARCHAR(32) PRIMARY KEY, `station` INT);")
	if err != nil {
		return err
	}

	return nil
}

/** Get the station, the token is assigned to. Return 0 if not token is found */
func (db *Persistence) GetStationToken(token string) (int ,error) {
	stmt, err := db.con.Prepare("SELECT `station` FROM `tokens` WHERE `token` = ? LIMIT 1")
	if err != nil {
		return 0, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(token)
	if err != nil {
		return 0, err
	}
	defer rows.Close()
	if rows.Next() {
		var station int
		rows.Scan(&station)
		return station, nil
	}
	return 0, nil
}

/** Inserts the given station and assign the ID to the given station parameter */
func (db *Persistence) InsertStation(station *Station) error {
	id, err := db.getHighestId("stations", "id")
	if err != nil {
		return err
	}

	// Create table
	tablename := "station_" + strconv.Itoa(id)
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `" + tablename + "` (`timestamp` INT PRIMARY KEY, `temperature` FLOAT, `humidity` FLOAT, `pressure` FLOAT);")
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
	defer rows.Close()
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
	defer rows.Close()
	if rows.Next() {
		station := Station{}
		rows.Scan(&station.Id, &station.Name, &station.Location, &station.Description)
		return station, nil
	} else {
		station.Id = 0
		return station, nil
	}
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
	defer rows.Close()
	for rows.Next() {
		datapoint := DataPoint{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Temperature, &datapoint.Humidity, &datapoint.Pressure)
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

func (db *Persistence) QueryStation(station int, t_min int64, t_max int64, limit int) ([]DataPoint, error) {
	datapoints := make([]DataPoint, 0)
	tablename := "station_" + strconv.Itoa(station)
	stmt, err := db.con.Prepare("SELECT `timestamp`,`temperature`,`humidity`,`pressure` FROM `" + tablename + "` WHERE `timestamp` >= ? AND `timestamp` <= ? ORDER BY `timestamp` DESC LIMIT ?")
	if err != nil {
		return datapoints, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(t_min, t_max, limit)
	if err != nil {
		return datapoints, err
	}
	defer rows.Close()
	for rows.Next() {
		datapoint := DataPoint{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Temperature, &datapoint.Humidity, &datapoint.Pressure)
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

/** Inserts the given datapoint to the database */
func (db *Persistence) InsertDataPoint(dp DataPoint) error {
	tablename := "station_" + strconv.Itoa(dp.Station)
	stmt, err := db.con.Prepare("INSERT INTO `" + tablename + "` (`timestamp`,`temperature`,`humidity`,`pressure`) VALUES (?,?,?,?) ON DUPLICATE KEY UPDATE `temperature` = ?, `humidity` = ?, `pressure` = ?;")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure, dp.Temperature, dp.Humidity, dp.Pressure)
	return err
}
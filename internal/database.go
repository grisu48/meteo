package meteo

import (
	"database/sql"
	"strconv"

	_ "github.com/mattn/go-sqlite3"
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

/* Ombrometer station reading */
type Rain struct {
	Station     int
	Timestamp   int64
	Millimeters float32
}

type Persistence struct {
	con *sql.DB
}

type Token struct {
	Token   string
	Station int
}

type Lightning struct {
	Station   int
	Timestamp int64
	Distance  float32
}

func OpenDb(filename string) (Persistence, error) {
	db := Persistence{}
	con, err := sql.Open("sqlite3", filename)
	if err != nil {
		return db, err
	}
	db.con = con
	return db, nil
}

func (db *Persistence) Close() {
	db.con.Close()
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
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `lightnings` (`station` INT, `timestamp` INT, `distance` REAL, PRIMARY KEY(`station`,`timestamp`));")
	if err != nil {
		return err
	}
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `ombrometers` (`id` INT PRIMARY KEY, `name` VARCHAR(64), `location` TEXT, `description` TEXT);")
	if err != nil {
		return err
	}

	return nil
}

/** Get the station, the token is assigned to. Returns Token.Station = 0 if not token is found */
func (db *Persistence) GetToken(token string) (Token, error) {
	ret := Token{Station: 0}
	stmt, err := db.con.Prepare("SELECT `token`,`station` FROM `tokens` WHERE `token` = ? LIMIT 1")
	if err != nil {
		return ret, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(token)
	if err != nil {
		return ret, err
	}
	defer rows.Close()
	if rows.Next() {
		rows.Scan(&ret.Token, &ret.Station)
		return ret, nil
	}
	return ret, nil
}

func (db *Persistence) GetStationTokens(station int) ([]Token, error) {
	ret := make([]Token, 0)
	stmt, err := db.con.Prepare("SELECT `token`,`station` FROM `tokens` WHERE `station` = ? LIMIT 1")
	if err != nil {
		return ret, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(station)
	if err != nil {
		return ret, err
	}
	defer rows.Close()
	for rows.Next() {
		tok := Token{}
		rows.Scan(&tok.Token, &tok.Station)
		ret = append(ret, tok)
	}
	return ret, nil
}

func (db *Persistence) InsertToken(token Token) error {
	stmt, err := db.con.Prepare("INSERT OR IGNORE INTO `tokens` (`token`,`station`) VALUES (?,?);")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(token.Token, token.Station)
	return err
}

func (db *Persistence) RemoveToken(token Token) error {
	stmt, err := db.con.Prepare("DELETE FROM `tokens` WHERE `token` = ?")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(token.Token)
	return err
}

/** Inserts the given station and assign the ID to the given station parameter */
func (db *Persistence) InsertStation(station *Station) error {
	id := station.Id
	var err error

	if id == 0 {
		id, err = db.getHighestId("stations", "id")
		if err != nil {
			return err
		}
		id += 1
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

func (db *Persistence) UpdateStation(station Station) error {
	stmt, err := db.con.Prepare("UPDATE `stations` SET `name`=?,`location`=?,`description`=? WHERE `id` = ?")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(station.Name, station.Location, station.Description, station.Id)
	return err
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
		datapoint.Station = station
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

/** Query given station within the given timespan. If limit <=0, the limit is set to 100000 */
func (db *Persistence) QueryStation(station int, t_min int64, t_max int64, limit int64, offset int64) ([]DataPoint, error) {
	datapoints := make([]DataPoint, 0)
	tablename := "station_" + strconv.Itoa(station)
	sql := "SELECT `timestamp`,`temperature`,`humidity`,`pressure` FROM `" + tablename + "` WHERE `timestamp` >= ? AND `timestamp` <= ? ORDER BY `timestamp` ASC LIMIT ? OFFSET ?"
	stmt, err := db.con.Prepare(sql)
	if err != nil {
		return datapoints, err
	}
	defer stmt.Close()
	if limit <= 0 {
		limit = 100000
	}
	rows, err := stmt.Query(t_min, t_max, limit, offset)
	if err != nil {
		return datapoints, err
	}
	defer rows.Close()
	for rows.Next() {
		datapoint := DataPoint{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Temperature, &datapoint.Humidity, &datapoint.Pressure)
		datapoint.Station = station
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

/** Inserts the given datapoint to the database */
func (db *Persistence) InsertDataPoint(dp DataPoint) error {
	tablename := "station_" + strconv.Itoa(dp.Station)
	stmt, err := db.con.Prepare("INSERT OR REPLACE INTO `" + tablename + "` (`timestamp`,`temperature`,`humidity`,`pressure`) VALUES (?,?,?,?);")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(dp.Timestamp, dp.Temperature, dp.Humidity, dp.Pressure)
	return err
}

/** Inserts the given lightning to the database */
func (db *Persistence) InsertLightning(light Lightning) error {
	stmt, err := db.con.Prepare("INSERT OR IGNORE INTO `lightnings` (`station`,`timestamp`,`distance`) VALUES (?,?,?);")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(light.Station, light.Timestamp, light.Distance)
	return err
}

func (db *Persistence) GetLightnings(limit int, offset int) ([]Lightning, error) {
	ret := make([]Lightning, 0)
	stmt, err := db.con.Prepare("SELECT `station`,`timestamp`,`distance` FROM `lightnings` ORDER BY `timestamp` DESC LIMIT ? OFFSET ?")
	if err != nil {
		return ret, err
	}
	defer stmt.Close()
	rows, err := stmt.Query(limit, offset)
	if err != nil {
		return ret, err
	}
	defer rows.Close()
	for rows.Next() {
		lightning := Lightning{}
		rows.Scan(&lightning.Station, &lightning.Timestamp, &lightning.Distance)
		ret = append(ret, lightning)
	}
	return ret, err
}

func (db *Persistence) GetOmbrometers() ([]Station, error) {
	stations := make([]Station, 0)
	rows, err := db.con.Query("SELECT `id`,`name`,`location`,`description` FROM `ombrometers`")
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

/** Inserts the given ombrometer station and assign the ID to the given station parameter */
func (db *Persistence) InsertOmbrometer(station *Station) error {
	id := station.Id
	var err error

	if id == 0 {
		id, err = db.getHighestId("ombrometers", "id")
		if err != nil {
			return err
		}
		id++
	}

	// Create table
	tablename := "ombrometer_" + strconv.Itoa(id)
	_, err = db.con.Exec("CREATE TABLE IF NOT EXISTS `" + tablename + "` (`timestamp` INT PRIMARY KEY, `millimeter` FLOAT);")
	if err != nil {
		return err
	}

	stmt, err := db.con.Prepare("INSERT INTO `ombrometers` (`id`, `name`, `location`, `description`) VALUES (?,?,?,?);")
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

func (db *Persistence) UpdateOmbrometer(station Station) error {
	stmt, err := db.con.Prepare("UPDATE `ombrometers` SET `name`=?,`location`=?,`description`=? WHERE `id` = ?")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(station.Name, station.Location, station.Description, station.Id)
	return err
}

func (db *Persistence) GetOmbrometer(id int) (Station, error) {
	station := Station{}

	stmt, err := db.con.Prepare("SELECT `id`,`name`,`location`,`description` FROM `ombrometers` WHERE `id` = ? LIMIT 1")
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

// GetOmbrometerReadings returns the most recent readings from the given station
func (db *Persistence) GetOmbrometerReadings(station int, limit int) ([]Rain, error) {
	datapoints := make([]Rain, 0)
	tablename := "ombrometer_" + strconv.Itoa(station)
	stmt, err := db.con.Prepare("SELECT `timestamp`,`millimeter` FROM `" + tablename + "` ORDER BY `timestamp` DESC LIMIT ?")
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
		datapoint := Rain{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Millimeters)
		datapoint.Station = station
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

// GetOmbrometerLastReading returns the last reading of the given station or an empty new Rain object
func (db *Persistence) GetOmbrometerLastReading(station int) (Rain, error) {
	dps, err := db.GetOmbrometerReadings(station, 1)
	if err != nil || len(dps) == 0 {
		return Rain{Station: station}, err
	} else {
		return dps[0], nil
	}
}

// QueryOmbrometer queries the given station within the given timespan. If limit <=0, the limit is set to 100000
func (db *Persistence) QueryOmbrometer(station int, t_min int64, t_max int64, limit int64, offset int64) ([]Rain, error) {
	datapoints := make([]Rain, 0)
	tablename := "ombrometer_" + strconv.Itoa(station)
	sql := "SELECT `timestamp`,`millimeter` FROM `" + tablename + "` WHERE `timestamp` >= ? AND `timestamp` <= ? ORDER BY `timestamp` ASC LIMIT ? OFFSET ?"
	stmt, err := db.con.Prepare(sql)
	if err != nil {
		return datapoints, err
	}
	defer stmt.Close()
	if limit <= 0 {
		limit = 100000
	}
	rows, err := stmt.Query(t_min, t_max, limit, offset)
	if err != nil {
		return datapoints, err
	}
	defer rows.Close()
	for rows.Next() {
		datapoint := Rain{}
		rows.Scan(&datapoint.Timestamp, &datapoint.Millimeters)
		datapoint.Station = station
		datapoints = append(datapoints, datapoint)
	}

	return datapoints, nil
}

// InsertRainMeasurement inserts a single rain measurement into the database
func (db *Persistence) InsertRainMeasurement(dp Rain) error {
	tablename := "ombrometer_" + strconv.Itoa(dp.Station)
	stmt, err := db.con.Prepare("INSERT OR REPLACE INTO `" + tablename + "` (`timestamp`,`millimeter`) VALUES (?,?);")
	if err != nil {
		return err
	}
	defer stmt.Close()
	_, err = stmt.Exec(dp.Timestamp, dp.Millimeters)
	return err
}

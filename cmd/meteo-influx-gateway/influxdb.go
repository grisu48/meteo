package main

import (
	"net/url"
	"time"

	client "github.com/influxdata/influxdb1-client"
)

type InfluxDB struct {
	database string
	client   *client.Client
}

func ConnectInfluxDB(remote string, username string, password string, database string) (InfluxDB, error) {
	var influx InfluxDB
	influx.database = database

	host, err := url.Parse(remote)
	if err != nil {
		return influx, err
	}
	conf := client.Config{
		URL:      *host,
		Username: username,
		Password: password,
	}
	influx.client, err = client.NewClient(conf)
	return influx, err
}

// Ping the InfluxDB server
func (influx *InfluxDB) Ping() (time.Duration, string, error) {
	return influx.client.Ping()
}

// Write a measurement into the database
func (influx *InfluxDB) Write(measurement string, tags map[string]string, fields map[string]interface{}) error {
	point := client.Point{
		Measurement: measurement,
		Tags:        tags,
		Fields:      fields,
		//Time:        time.Now(),
		Precision: "s",
	}
	pts := make([]client.Point, 0)
	pts = append(pts, point)
	bps := client.BatchPoints{
		Points:   pts,
		Database: influx.database,
		//RetentionPolicy: "default",
	}
	_, err := influx.client.Write(bps)
	return err
}

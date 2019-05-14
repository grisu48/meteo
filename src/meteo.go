/*
 * Simple CLI meteo client
 */
package main

import (
	"fmt"
	"os"
	"strings"
	"strconv"
	"time"
	"errors"
	"io/ioutil"
	"net/http"
)


type Station struct {
	Id int
	Timestamp int64
	Temperature float32
	Humidity float32
	Pressure float32
}

func httpGet(url string) ([]byte, error) {
	ret := make([]byte, 0)
	if len(url) == 0 { return ret, errors.New("Empty URL")}
	resp, err := http.Get(url)
	if err != nil { return ret, err }
	if !strings.HasPrefix(resp.Status, "200 ") {
		return ret, errors.New(fmt.Sprintf("Http error %s", resp.Status))
	}
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	return body, err
}

func request(hostname string) ([]Station, error) {
	ret := make([]Station, 0)

	url := hostname

	// Add https if nothing is there
	autoHttps := false
	if !strings.Contains(hostname, "://") {
		autoHttps = true
		url = "https://" + hostname + "/meteo"
	}
	if len(url) == 0 { return ret, errors.New("Empty URL")}
	if url[len(url)-1] == '/' {
		url = url[:len(url)-1]
	}

	body, err := httpGet(url + "/current")
	if err != nil { 
		// Try with http instead of https, if we have added https automatically
		if autoHttps {
			//fmt.Fprintln(os.Stderr, "  # Warning: Fallback to unencrypted http!")
			url = strings.Replace(url, "https://", "http://", 1)
			body, err = httpGet(url + "/current")
			if err != nil { return ret, err }
		} else {
			return ret, err
		}
	}


	response := strings.TrimSpace(string(body))
	// Parse response lines
	for _, line := range(strings.Split(response, "\n")) {
		line = strings.TrimSpace(line)
		if len(line) == 0 { continue }
		if line[0] == '#' { continue }
		params := strings.Split(line, ",")
		if len(params) == 5 {
			station := Station{}
			var err error
			var f float64
			station.Id, err = strconv.Atoi(params[0])
			if err != nil { continue }
			station.Timestamp, err = strconv.ParseInt(params[1], 10, 64)
			if err != nil { continue }
			f, err = strconv.ParseFloat(params[2], 32)
			if err != nil { continue }
			station.Temperature = float32(f)
			f, err = strconv.ParseFloat(params[3], 32)
			if err != nil { continue }
			station.Humidity = float32(f)
			f, err = strconv.ParseFloat(params[4], 32)
			if err != nil { continue }
			station.Pressure = float32(f)
			ret = append(ret, station)
		}
	}

	return ret, nil
}

func main() {
	args := os.Args[1:]
	if len(args) == 0 {
		fmt.Println("Usage: %s REMOTE\n", os.Args[0])
		fmt.Println("       REMOTE defines a running meteo webserver\n")
		os.Exit(1)
	}

	for _, hostname := range(args) {
		fmt.Println(hostname)
		stations, err := request(hostname)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %s\n", err)
		}
		for _, station := range(stations) {
			time := time.Unix(station.Timestamp, 0)
			fmt.Printf("  * %3d |%19s| %5.2f degree, %5.2f %% rel. Humidity, %06.0f hPa\n", station.Id, time.Format("2006-01-02-15:04:05"), station.Temperature, station.Humidity, station.Pressure)
		}
	}

}

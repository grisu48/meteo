/*
 * Meteo program to read out connected ServerNode and push the data via HTTP to
 * a given server
 */
package main

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"

	"github.com/BurntSushi/toml"
	"github.com/jacobsa/go-serial/serial"
)

type Config struct {
	Stations map[string]SerialStation `toml:"Serial"`
}

type SerialStation struct {
	Device string
	Baud   uint
	Remote string
	Token  string
}

type DataPoint struct {
	Station     int
	Name        string
	Temperature float64
	Humidity    float64
	Pressure    float64
}

var cf Config

func parse(line string) (DataPoint, error) {
	dp := DataPoint{}

	line = strings.TrimSpace(line)
	if line == "" {
		return dp, nil
	}
	// XXX: Improve this: Espaced name
	split := strings.Split(line, " ")
	// Expected format: '1 "Meteo-Station" 2601 27 94366' (without '')
	if len(split) != 5 {
		return dp, errors.New("Illegal packet")
	}

	var err error
	dp.Station, err = strconv.Atoi(split[0])
	if err != nil {
		return dp, err
	}
	dp.Name = split[1]
	dp.Temperature, err = strconv.ParseFloat(split[2], 10)
	if err != nil {
		return dp, err
	}
	dp.Temperature /= 100.0
	dp.Humidity, err = strconv.ParseFloat(split[3], 10)
	if err != nil {
		return dp, err
	}
	dp.Pressure, err = strconv.ParseFloat(split[4], 10)
	if err != nil {
		return dp, err
	}

	return dp, nil
}

func serialOpen(device string, baud uint) (io.ReadWriteCloser, error) {
	// Set up options.
	options := serial.OpenOptions{
		PortName:        device,
		BaudRate:        baud,
		DataBits:        8,
		StopBits:        1,
		MinimumReadSize: 4,
	}

	// Open the port.
	return serial.Open(options)
}

func handleSerial(station SerialStation) {
	fmt.Println(station.Remote)

	serial, err := serialOpen(station.Device, station.Baud)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error opening %s: %s\n", station.Device, err)
		return
	}
	defer serial.Close()
	data := make([]byte, 1024)
	for { // Read continously
		n, err := serial.Read(data)
		if n == 0 {
			break
		}
		if err != nil {
			if err == io.EOF {
				break
			}
			panic(err)
		}

		lines := strings.Split(string(data[:n]), "\n")
		for _, line := range lines {
			line = strings.TrimSpace(line)
			dp, err := parse(line)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error reading: %s\n", err)
			} else {
				if dp.Station == 0 {
					continue
				} // Ignore, because invalid
				err := publish(dp, station.Remote, station.Token)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error publishing: %s\n", err)
				} else {
					fmt.Println(line)
				}
			}
		}
	}
}

func main() {
	// Configuration - set defaults and read from file
	toml.DecodeFile("/etc/meteo/gateway.toml", &cf)
	toml.DecodeFile("meteo.toml", &cf)
	toml.DecodeFile("meteo-gateway.toml", &cf)

	if len(cf.Stations) == 0 {
		fmt.Fprintf(os.Stderr, "No station defined\n")
		os.Exit(1)
	}

	// Create thread for each serial port
	var wg sync.WaitGroup
	for _, serial := range cf.Stations {
		wg.Add(1)
		go func(station SerialStation) {
			defer wg.Done()
			handleSerial(station)
		}(serial)
	}
	wg.Wait()
}

func publish(dp DataPoint, remote string, token string) error {
	// Build json packet
	json := fmt.Sprintf("{\"token\":\"%s\",\"T\":%.2f,\"Hum\":%.2f,\"P\":%.2f}", token, dp.Temperature, dp.Humidity, dp.Pressure)
	//fmt.Println(json)

	// Prepare POST request
	hc := http.Client{}
	addr := fmt.Sprintf("%s/station/%d", remote, dp.Station)
	req, err := http.NewRequest("POST", addr, strings.NewReader(json))
	if err != nil {
		return err
	}
	req.Header.Add("Content-Type", "application/json")
	resp, err := hc.Do(req)
	if err != nil {
		return err
	}
	body, err := ioutil.ReadAll(resp.Body)
	if resp.StatusCode != 200 {
		fmt.Fprintf(os.Stderr, "POST returned status %d\n%s\n\n", resp.StatusCode, string(body))
	}

	// All good
	return nil
}

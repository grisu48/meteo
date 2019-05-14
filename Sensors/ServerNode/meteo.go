/*
 * Meteo program to read out connected ServerNode and push the data via HTTP to 
 * a given server
 */
package main

import (
	"fmt"
	"net/http"
	"io"
	"io/ioutil"
	"os"
	"strings"
	"strconv"
	"errors"
	"github.com/jacobsa/go-serial/serial"
	"github.com/BurntSushi/toml"
)

type Config struct {
	Device string
	Baud uint
	Remote string
	Token string
}

type DataPoint struct {
	Station int
	Name string
	Temperature float64
	Humidity float64
	Pressure float64
}

var cf Config

func parse(line string) (DataPoint, error) {
	dp := DataPoint{}

	line = strings.TrimSpace(line)
	if line == "" { return dp, nil }
	// XXX: Improve this: Espaced name
	split := strings.Split(line, " ")
	// Expected format: '1 "Astro-Cluster" 2601 27 94366' (without '')
	if len(split) != 5 { return dp, errors.New("Illegal packet") }

	var err error
	dp.Station, err = strconv.Atoi(split[0])
	if err != nil { return dp, err }
	dp.Name = split[1]
	dp.Temperature, err = strconv.ParseFloat(split[2], 10)
	if err != nil { return dp, err }
	dp.Temperature /= 100.0
	dp.Humidity, err = strconv.ParseFloat(split[3], 10)
	if err != nil { return dp, err }
	dp.Pressure, err = strconv.ParseFloat(split[4], 10)
	if err != nil { return dp, err }

	return dp,nil
}

func serialOpen(device string, baud uint) (io.ReadWriteCloser, error) {
	 // Set up options.
    options := serial.OpenOptions{
      PortName: device,
      BaudRate: baud,
      DataBits: 8,
      StopBits: 1,
      MinimumReadSize: 4,
    }

    // Open the port.
    return serial.Open(options)
}

func main() {
	// Configuration - set defaults and read from file
	cf.Device = "/dev/ttyUSB0"
	cf.Baud = 115200
	cf.Remote = "http://127.0.0.1:8802"
	cf.Token = "00000000000000000000000000000000"
	toml.DecodeFile("/etc/meteo.toml", &cf)
	toml.DecodeFile("meteo.toml", &cf)

	// Connect serial port
	serial, err := serialOpen(cf.Device, cf.Baud)
	if err != nil {
		panic(err)
	}
    defer serial.Close()
    data := make([]byte, 1024)
    for {		// Read continously
    	n, err := serial.Read(data)
    	if n == 0 { break }
        if err != nil {
            if err == io.EOF { break }
            panic(err)
        }
        
        lines := strings.Split(string(data[:n]), "\n")
        for _, line := range(lines) {
        	line = strings.TrimSpace(line)
	        dp, err := parse(line)
	        if err != nil {
	        	fmt.Fprintf(os.Stderr, "Error reading: ", err)
	        } else {
	        	if dp.Station == 0 { continue }		// Ignore, because invalid
	        	err := publish(dp)
	        	if err != nil {
	        		fmt.Fprintf(os.Stderr, "Error publishing: ", err)
	        	} else {
	        		fmt.Println(line)
	        	}
	        }
        }
    }
}

func publish(dp DataPoint) (error) {
	// Build json packet
	json := fmt.Sprintf("{\"token\":\"%s\",\"T\":%.2f,\"Hum\":%.2f,\"P\":%.2f}", cf.Token, dp.Temperature, dp.Humidity, dp.Pressure)
	//fmt.Println(json)

	// Prepare POST request
	hc := http.Client{}
	addr := fmt.Sprintf("%s/station/%d", cf.Remote, dp.Station)
	req, err := http.NewRequest("POST", addr, strings.NewReader(json))
	if err != nil { return err }
	req.Header.Add("Content-Type", "application/json")
	resp, err := hc.Do(req)
	if err != nil { return err }
	body, err := ioutil.ReadAll(resp.Body)
	if resp.StatusCode != 200 {
		fmt.Fprintf(os.Stderr, "POST returned status %d\n%s\n\n", resp.StatusCode, strings.TrimSpace(string(body)))
	}

	// All good
	return nil
}

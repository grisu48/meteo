/*
 * Simple CLI meteo client
 */
package main

import (
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/BurntSushi/toml"
)

// Terminal color codes
const KNRM = "\x1B[0m"
const KRED = "\x1B[31m"
const KGRN = "\x1B[32m"
const KYEL = "\x1B[33m"
const KBLU = "\x1B[34m"
const KMAG = "\x1B[35m"
const KCYN = "\x1B[36m"
const KWHT = "\x1B[37m"

type LocalStation struct {
	Id          int
	Name        string
	Timestamp   int64
	Temperature float32
	Humidity    float32
	Pressure    float32
}

type StationMeta struct {
	Id          int
	Name        string
	Location    string
	Description string
}

/* Warning and error ranges */
type Ranges struct {
	T_Range   []float32 `toml:"temp_range"`
	Hum_Range []float32 `toml:"hum_range"`
	//P_Range []float32 `toml:"p_range"`
}

type tomlClientConfig struct {
	DefaultRemote string `toml:"DefaultRemote"`

	Remotes       map[string]Remote `toml:"Remotes"`
	StationRanges map[string]Ranges `toml:"Ranges"`
}

type Remote struct {
	Remote string
}

func httpGet(url string) ([]byte, error) {
	ret := make([]byte, 0)
	if len(url) == 0 {
		return ret, errors.New("Empty URL")
	}
	resp, err := http.Get(url)
	if err != nil {
		return ret, err
	}
	if !strings.HasPrefix(resp.Status, "200 ") {
		return ret, errors.New(fmt.Sprintf("Http error %s", resp.Status))
	}
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	return body, err
}

func GetStations(baseUrl string) (map[int]StationMeta, error) {
	ret := make(map[int]StationMeta, 0)
	url := baseUrl + "/stations"
	body, err := httpGet(url)
	if err != nil {
		return ret, err
	}
	response := strings.TrimSpace(string(body))
	for _, line := range strings.Split(response, "\n") {
		if len(line) == 0 || line[0] == '#' {
			continue
		}
		params := strings.Split(line, ",")
		if len(params) == 4 {
			var err error
			station := StationMeta{}
			station.Id, err = strconv.Atoi(params[0])
			if err != nil {
				continue
			}
			station.Name = params[1]
			station.Location = params[2]
			station.Description = params[3]
			ret[station.Id] = station
		}
	}
	return ret, nil
}

func request(hostname string) ([]LocalStation, error) {
	ret := make([]LocalStation, 0)

	url := hostname

	// Add https if nothing is there
	autoHttps := false
	if !strings.Contains(hostname, "://") {
		autoHttps = true
		url = "https://" + hostname + "/meteo"
	}
	if len(url) == 0 {
		return ret, errors.New("Empty URL")
	}
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
			if err != nil {
				return ret, err
			}
		} else {
			return ret, err
		}
	}

	stations, _ := GetStations(url) // Ignore errors here

	response := strings.TrimSpace(string(body))
	// Parse response lines
	for _, line := range strings.Split(response, "\n") {
		line = strings.TrimSpace(line)
		if len(line) == 0 {
			continue
		}
		if line[0] == '#' {
			continue
		}
		params := strings.Split(line, ",")
		if len(params) == 5 {
			station := LocalStation{}
			var err error
			var f float64
			station.Id, err = strconv.Atoi(params[0])
			if err != nil {
				continue
			}
			station.Timestamp, err = strconv.ParseInt(params[1], 10, 64)
			if err != nil {
				continue
			}
			f, err = strconv.ParseFloat(params[2], 32)
			if err != nil {
				continue
			}
			station.Temperature = float32(f)
			f, err = strconv.ParseFloat(params[3], 32)
			if err != nil {
				continue
			}
			station.Humidity = float32(f)
			f, err = strconv.ParseFloat(params[4], 32)
			if err != nil {
				continue
			}
			station.Pressure = float32(f)
			station.Name = stations[station.Id].Name
			ret = append(ret, station)
		}
	}

	return ret, nil
}

func printHelp() {
	fmt.Printf("Usage: %s [OPTIONS] REMOTE [REMOTE] ...\n", os.Args[0])
	fmt.Println("       REMOTE defines a running meteo webserver")
	fmt.Printf(" e.g. %s http://meteo.local/\n", os.Args[0])
	fmt.Printf("      %s https://monitor-server.local/meteo/\n", os.Args[0])
	fmt.Printf("\n")
	fmt.Printf("OPTIONS\n")
	fmt.Printf("  -h, --help            Print this help message\n")
	fmt.Printf("\n")
	fmt.Println("https://github.com/grisu48/meteo")
}

func main() {
	args := os.Args[1:]
	hosts := make([]string, 0)

	// Read configuration
	var cf tomlClientConfig
	toml.DecodeFile("/etc/meteo.toml", &cf)
	toml.DecodeFile("meteo.toml", &cf)

	// Parse arguments
	for _, arg := range args {
		if arg == "" {
			continue
		}
		if arg[0] == '-' {
			// Check for special parameters
			if arg == "-h" || arg == "--help" {
				printHelp()
				os.Exit(0)
			} else {
				fmt.Fprintf(os.Stderr, "Illegal argument: %s\n", arg)
				os.Exit(1)
			}
		} else {
			hosts = append(hosts, arg)
		}
	}

	if len(hosts) == 0 {
		if cf.DefaultRemote == "" {
			printHelp()
			os.Exit(1)
		} else {
			hosts = append(hosts, cf.DefaultRemote)
		}
	}

	for _, hostname := range hosts {

		if val, ok := cf.Remotes[hostname]; ok {
			hostname = val.Remote
		}

		fmt.Println(hostname)
		stations, err := request(hostname)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %s\n", err)
		}
		for _, station := range stations {
			timestamp := time.Unix(station.Timestamp, 0)

			/* ==== Building the line with colors ==== */
			// Yes, this has become a bit messy
			fmt.Printf("  * %3d %-22s ", station.Id, station.Name)
			// Set color for time
			if time.Since(timestamp).Minutes() > 5 {
				fmt.Printf(KRED)
			} else {
				fmt.Printf(KGRN)
			}
			fmt.Printf("%19s", timestamp.Format("2006-01-02-15:04:05"))
			fmt.Printf(KNRM)
			fmt.Printf("   ")
			// Check if we have some custom ranges
			if val, ok := cf.StationRanges[station.Name]; ok {
				if len(val.T_Range) == 4 {
					if station.Temperature <= val.T_Range[0] || station.Temperature >= val.T_Range[3] {
						fmt.Printf(KRED)
					} else if station.Temperature <= val.T_Range[1] || station.Temperature >= val.T_Range[2] {
						fmt.Printf(KYEL)
					} else {
						fmt.Printf(KGRN)
					}
				}
				fmt.Printf("%5.2fC", station.Temperature)
				fmt.Printf(KNRM)
				fmt.Printf("|")
				if len(val.Hum_Range) == 4 {
					if station.Humidity <= val.Hum_Range[0] || station.Humidity >= val.Hum_Range[3] {
						fmt.Printf(KRED)
					} else if station.Humidity <= val.Hum_Range[1] || station.Humidity >= val.Hum_Range[2] {
						fmt.Printf(KYEL)
					} else {
						fmt.Printf(KGRN)
					}
				}
				fmt.Printf("%5.2f %%rel", station.Humidity)
				fmt.Printf(KNRM)
				fmt.Printf("|%6.0fhPa\n", station.Pressure)
			} else {
				fmt.Printf("%5.2fC|%5.2f %%rel|%6.0fhPa\n", station.Temperature, station.Humidity, station.Pressure)
			}
		}
	}

}

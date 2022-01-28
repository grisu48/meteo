package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"gopkg.in/ini.v1"
)

// Config is the internal singleton configuration for this program
type Config struct {
	MqttHost       string `ini:"mqtt,remote"`
	MqttClientID   string `ini:"mqtt,clientid"`
	InfluxHost     string `ini:"influxdb,remote"`
	InfluxUsername string `ini:"influxdb,username"`
	InfluxPassword string `ini:"influxdb,password"`
	InfluxDatabase string `ini:"influxdb,database"`
	Verbose        bool
}

func (c *Config) loadIni(filename string) error {
	cfg, err := ini.Load(filename)
	if err != nil {
		return err
	}

	mqtt := cfg.Section("mqtt")
	mqtthost := mqtt.Key("remote").String()
	if mqtthost != "" {
		c.MqttHost = mqtthost
	}
	mqttclientid := mqtt.Key("clientid").String()
	if mqttclientid != "" {
		c.MqttClientID = mqttclientid
	}
	influx := cfg.Section("influxdb")
	influxhost := influx.Key("remote").String()
	if influxhost != "" {
		c.InfluxHost = influxhost
	}
	influxuser := influx.Key("username").String()
	if influxuser != "" {
		c.InfluxUsername = influxuser
	}
	influxpass := influx.Key("password").String()
	if influxpass != "" {
		c.InfluxPassword = influxpass
	}
	influxdb := influx.Key("database").String()
	if influxdb != "" {
		c.InfluxDatabase = influxdb
	}

	return nil
}

var config Config

var influx InfluxDB

func assembleJson(node int, data map[string]interface{}) string {
	ret := fmt.Sprintf("node %d: {", node)
	first := true
	for k, v := range data {
		if first {
			first = false
		} else {
			ret += ", "
		}
		ret += fmt.Sprintf("\"%s\":%f", k, v)
	}
	ret += "}"
	return ret
}

func received(msg mqtt.Message) {
	data := make(map[string]interface{}, 0)
	if err := json.Unmarshal(msg.Payload(), &data); err != nil {
		fmt.Fprintf(os.Stderr, "json unmarshall error: %s\n", err)
		return
	}

	// We don't log the name, remove it, if present
	if _, ok := data["name"]; ok {
		delete(data, "name")
	}
	// ID is taken from the topic
	if _, ok := data["id"]; ok {
		delete(data, "id")
	}

	// Parse node ID from topic
	nodeID, err := strconv.Atoi(msg.Topic()[6:])
	if err != nil {
		fmt.Fprintf(os.Stderr, "invalid meteo id\n")
		return
	}

	// Write to InfluxDB
	for k, v := range data {
		tags := map[string]string{"node": fmt.Sprintf("%d", nodeID)}
		f, err := strconv.ParseFloat(fmt.Sprintf("%f", v), 32)
		if err != nil {
			fmt.Fprintf(os.Stderr, "non-float value received: %s\n", err)
			return
		}
		fields := map[string]interface{}{"value": f}
		if err := influx.Write(k, tags, fields); err != nil {
			fmt.Fprintf(os.Stderr, "cannot write to influxdb: %s\n", err)
			return
		}
	}

	// OK
	if config.Verbose {
		fmt.Println(assembleJson(nodeID, data))
	}
}

// awaits SIGINT or SIGTERM
func awaitTerminationSignal() {
	sigs := make(chan os.Signal, 1)
	done := make(chan bool, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigs
		fmt.Println(sig)
		done <- true
	}()
	<-done
}

func printUsage() {
	fmt.Println("meteo-influxdb-gateway")
	fmt.Printf("Usage: %s [OPTIONS]\n", os.Args[0])
	fmt.Println("OPTIONS")
	fmt.Println("  -h,--help             Display this help message")
	fmt.Println("  -c,--config FILE      Load config file")
	fmt.Println("  -v, --verbose         Verbose mode")
	fmt.Println("  --mqtt MQTT           Set mqtt server")
	fmt.Println("  --mqtt-client-id ID   Set mqtt client id")
	fmt.Println("  --influx HOST         Set influxdb hostname")
	fmt.Println("  --username USER       Set influxdb username")
	fmt.Println("  --password PASS       Set influxdb password")
	fmt.Println("  --database DB         Set influxdb database")

}

func fileExists(filename string) bool {
	if _, err := os.Stat(filename); os.IsNotExist(err) {
		return false
	}
	return true
}

func main() {
	var err error
	// Default settings
	config.MqttHost = "127.0.0.1"
	config.InfluxHost = "http://127.0.0.1:8086"
	config.InfluxUsername = "meteo"
	config.InfluxPassword = ""
	config.InfluxDatabase = "meteo"
	config.Verbose = false

	configFile := "/etc/meteo/meteo-influx-gateway.ini"
	if fileExists(configFile) {
		if err := config.loadIni(configFile); err != nil {
			fmt.Fprintf(os.Stderr, "error loading ini file %s: %s\n", configFile, err)
			os.Exit(1)
		}
	}

	args := os.Args[1:]
	for i := 0; i < len(args); i++ {
		arg := strings.TrimSpace(args[i])
		if arg == "" {
			continue
		} else if arg[0] == '-' {
			last := i >= len(args)-1
			if arg == "-h" || arg == "--help" {
				printUsage()
				return
			} else if arg == "-c" || arg == "--config" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: config file\n")
					os.Exit(1)
				}
				i++
				configFile = args[i]
				if err := config.loadIni(configFile); err != nil {
					fmt.Fprintf(os.Stderr, "error loading ini file %s: %s\n", configFile, err)
					os.Exit(1)
				}
			} else if arg == "-v" || arg == "--verbose" {
				config.Verbose = true
			} else if arg == "--mqtt" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: mqtt remote\n")
					os.Exit(1)
				}
				i++
				config.MqttHost = args[i]
			} else if arg == "--mqtt-client-id" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: mqtt-client-id remote\n")
					os.Exit(1)
				}
				i++
				config.MqttClientID = args[i]
			} else if arg == "--influx" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: influx remote\n")
					os.Exit(1)
				}
				i++
				config.InfluxHost = args[i]
			} else if arg == "--username" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: influx username\n")
					os.Exit(1)
				}
				i++
				config.InfluxUsername = args[i]
			} else if arg == "--password" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: influx password\n")
					os.Exit(1)
				}
				i++
				config.InfluxPassword = args[i]
			} else if arg == "--database" {
				if last {
					fmt.Fprintf(os.Stderr, "Missing argument: influx database\n")
					os.Exit(1)
				}
				i++
				config.InfluxDatabase = args[i]
			}
		} else {
			fmt.Fprintf(os.Stderr, "Invalid argument: %s\n", arg)
			os.Exit(1)
		}
	}

	// Connect InfluxDB
	influx, err = ConnectInfluxDB(config.InfluxHost, config.InfluxUsername, config.InfluxPassword, config.InfluxDatabase)
	if err != nil {
		fmt.Fprintf(os.Stderr, "cannot connect to influxdb: %s\n", err)
		os.Exit(1)
	}
	if ping, version, err := influx.Ping(); err != nil {
		fmt.Fprintf(os.Stderr, "cannot ping influxdb: %s\n", err)
		os.Exit(1)
	} else {
		if config.Verbose {
			fmt.Printf("influxdb connected: v%s (Ping: %d ms)\n", version, ping.Milliseconds())
		}
	}

	// Connect to mqtt server
	mqtt := MQTT{}
	mqtt.ClientID = config.MqttClientID
	mqtt.Verbose = config.Verbose
	mqtt.Callback = received
	if err := mqtt.Connect(config.MqttHost, 1883); err != nil {
		fmt.Fprintf(os.Stderr, "mqtt error: %s\n", err)
		os.Exit(1)
	}
	fmt.Println("meteo-mqtt-influx gateway is up and running")

	awaitTerminationSignal()
}

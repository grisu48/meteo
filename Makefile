default: all
all: meteo-influx-gateway

install: all

## === Builds =============================================================== ##

meteo-influx-gateway: cmd/meteo-influx-gateway/meteo-mqtt-influxdb.go cmd/meteo-influx-gateway/influxdb.go cmd/meteo-influx-gateway/mqtt.go
	go build -o $@ $^

## === Tests ================================================================ ##


default: all
all: meteo meteod influxgateway $(SUBDIRS)

install: all
	install meteo /usr/local/bin/
	install meteod /usr/local/bin/

## ==== Easy requirement install ============================================ ##

# requirements for meteod (server)
req:
	go get "github.com/BurntSushi/toml"
	go get "github.com/gorilla/mux"
	go get "github.com/mattn/go-sqlite3"
	go get "github.com/eclipse/paho.mqtt.golang"
# requirements for meteo (client)
req-meteo:
	go get "github.com/BurntSushi/toml"
req-gateway:
	go get "github.com/BurntSushi/toml"
	go get "github.com/jacobsa/go-serial/serial"


## === Builds =============================================================== ##

meteo: cmd/meteo/meteo.go
	go build $^
meteod: cmd/meteod/meteod.go
	go build $^
gateway: cmd/gateway/gateway.go
	go build $^
influxgateway: cmd/influxgateway/meteo-mqtt-influxdb.go cmd/influxgateway/influxdb.go cmd/influxgateway/mqtt.go
	go build -o meteo-influx-gateway $^

## === Tests ================================================================ ##

test:	internal/database.go
	go test -v ./...

$(SUBDIRS):
	$(MAKE) -C $@

default: all
all: meteo meteod $(SUBDIRS)

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


## === Builds =============================================================== ##

meteo: cmd/meteo/meteo.go
	go build $<
meteod: cmd/meteod/meteod.go
	go build $<
gateway: cmd/gateway/gateway.go
	go build $<

## === Tests ================================================================ ##

test:	internal/database.go
	go test -v ./...

$(SUBDIRS):
	$(MAKE) -C $@

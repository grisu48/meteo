SUBDIRS := gateway

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

meteo: meteo.go
	go build meteo.go
meteod: meteod.go database.go
	go build meteod.go database.go

## === Tests ================================================================ ##

test:	meteod.go database.go database__test.go
	go test -v database__test.go database.go

$(SUBDIRS):
	$(MAKE) -C $@

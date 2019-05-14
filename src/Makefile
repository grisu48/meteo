default: all
all: meteo meteod

install: all
	install meteo /usr/local/bin
	install meteod /usr/localbin

meteo: meteo.go
	go build meteo.go
meteod: meteod.go database.go
	go build meteod.go database.go

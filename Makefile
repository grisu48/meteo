SUBDIRS := gateway

default: all
all: meteo meteod $(SUBDIRS)

install: all
	install meteo /usr/local/bin/
	install meteod /usr/local/bin/

meteo: meteo.go
	go build meteo.go
meteod: meteod.go database.go
	go build meteod.go database.go

test:	meteod.go database.go database__test.go
	go test -v database__test.go database.go

$(SUBDIRS):
	$(MAKE) -C $@

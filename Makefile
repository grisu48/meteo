SUBDIRS := gateway

default: all
all: meteo meteod $(SUBDIRS)

install: all
	install meteo /usr/local/bin
	install meteod /usr/localbin

meteo: meteo.go
	go build meteo.go
meteod: meteod.go database.go
	go build meteod.go database.go

$(SUBDIRS):
	$(MAKE) -C $@

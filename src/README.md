# meteo

This is the main meteo server program, written in Go.

# Server

## Configuration

Currently manually. See `meteo.toml`

## Security

Pushing stations need a access `token`, that identifies where the data belongs to.

# Client

There is currently a very simple CLI client available: `meteo`

    meteo REMOTE
    
    $ meteo http://meteo-service.local/
      *   1 meteo-cluster          2019-05-14-17:24:01   22.51C|23.00 %rel| 95337hPa


# meteo Server

This is the `meteod` server program. It runs as a background daemon and collects sensor data coming in via mosquitto, writes them to a sqlite3 database and provides a pretty simple web-interface for querying data.

The web-interface is ment as a simple request-API for other programs. No extensive visualisation tools are provided yet and possibly will never be provided.

# Building

On most Linux systems a simple

    make
    sudo make install

should do the job. On FreeBSD system you have to manually add `-lexecinfo` to `LIBS` in the `Makefile`. It has been commented there out, look for the following lines

    LIBS=-pthread -lsqlite3 -lmosquitto
    # To compile on FreeBSD systems, also add "-lexecinfo" to LIBS:
    # LIBS=-pthread -lsqlite3 -lmosquitto -lexecinfo

Tested on FreeBSD 11.1 Release #0 r321309, and all additonal packets coming from the pkg system.

Tested under Ubuntu 16.04 and Debian strech.

**Have fun!**

# Usage

For now, unfortunately no usage documentation is available. Please be patient, it will come soon!

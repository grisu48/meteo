#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Simple python script to migrate data from a mysql database to a sqlite database

import os
import sys
import MySQLdb
import sqlite3


class Station :
	def __init__(self, s_id=0, name="", location="", description="") :
		self.id = s_id
		self.name = name
		self.location = location
		self.description = description
	
	def __str__(self) : return "(%2d) %s, %s, %s" % (self.id, self.name, self.location, self.description)

class Token :
	def __init__(self, token="", station=0) :
		self.token = token
		self.station = station

class DataPoint :
	def __init__(self, timestamp=0, station=0, temperature=0.0, humidity=0.0, pressure=0.0) :
		self.timestamp = timestamp
		self.station = station
		self.temperature = temperature
		self.humidity = humidity
		self.pressure = pressure



class MyMeteo :
	'''
	Class for accessing the deprecated meteo mysql database
	'''
	def __init__(self, hostname, database, username, password) :
		self.hostname = hostname
		self.database = database
		
		self._db = MySQLdb.connect(hostname, username, password, database)
	
	def db_version(self) :
		cursor = self._db.cursor()
		cursor.execute("SELECT VERSION()")
		return cursor.fetchone()
		
	def close(self) :
		self._db.close()
		
	def stations(self) :
		ret = []
		cursor = self._db.cursor()
		cursor.execute("SELECT `id`,`name`,`location`,`description` FROM `stations`")
		rows = cursor.fetchall()
		for row in rows :
			ret.append(Station(row[0], row[1], row[2], row[3]))
		return ret
		
	def tokens(self) : 
		ret = []
		cursor = self._db.cursor()
		cursor.execute("SELECT `token`,`station` FROM `tokens`")
		rows = cursor.fetchall()
		for row in rows :
			ret.append(Token(row[0], row[1]))
		return ret
		
	def count_datapoints(self, station) :
		ret = []
		cursor = self._db.cursor()
		cursor.execute("SELECT count(*) AS `count` FROM `station_%d`" % (station))
		return int(cursor.fetchone()[0])

class MeteoDB :
	def __init__(self, filename) :
		self.filename = filename
		self._con = sqlite3.connect(filename)
		
		
	def prepare(self) :
		c = self._con.cursor()
		c.execute("CREATE TABLE IF NOT EXISTS `stations` (`id` INT PRIMARY KEY, `name` VARCHAR(64), `location` TEXT, `description` TEXT);")
		c.execute("CREATE TABLE IF NOT EXISTS `tokens` (`token` VARCHAR(32) PRIMARY KEY, `station` INT);")
		
		
	def insertStation(self, station) :
		c = self._con.cursor()
		
		
	def close(self) : self._con.close()







def getPassword() :
	try :
		import getpass
		return getpass.getpass("Password> ")
	except ImportError as e:
		sys.stderr.write("ImportError: %s\n" % str(e))
		sys.stderr.flush()
		print("Falling back to default input")
		print("WARNING: Password will be shown in cleartext on prompt")
		return input("Password (unprotected)> ")

if __name__ == "__main__" :
	dbfilename = None
	confirm = True
	if len(sys.argv) > 1 : dbfilename = sys.argv[1]
	
	print("Configuring MySQL Database:")
	db_hostname = input("Hostname> ")
	db_database = input("Database> ")
	db_username = input("Username> ")
	db_password = getPassword()
	if dbfilename is None : dbfilename = input("Filename> ")
	
	# Connect to MySQL
	db, sq = None, None
	try :
		db = MyMeteo(db_hostname, db_username, db_password, db_database)
		print("Database connected : %s " % db.db_version())
		stations = db.stations()
		print("Fetched %d stations:" % (len(stations)))
		datapoints = 0
		for station in stations :
			dps = db.count_datapoints(station.id)
			datapoints += dps
			print("\t%s [%d datapoints[" % (station, dps))
		print("Counter %d datapoints in all stations" % (datapoints))
		tokens = db.tokens()
		print("Fetched %d tokens" % (len(tokens)))
		
		
		while confirm :
			yes = input("Continue? [Y/n] ").lower()
			if yes in ["no", "n"] : sys.exit(1)
			if yes in ["yes", "y"] : break
		
		print("Creating %s ... " % (dbfilename))
		sq = MeteoDB(dbfilename)
		
		
	finally :
		if not db is None : db.close()
		if not sq is None : sq.close()

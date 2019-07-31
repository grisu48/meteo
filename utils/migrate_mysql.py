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
		cursor.execute("SELECT count(*) AS `count` FROM `station_%d`" % (int(station)))
		return int(cursor.fetchone()[0])
		
	def datapoints(self, station, limit=10000, offset=0) :
		ret = []
		cursor = self._db.cursor()
		cursor.execute("SELECT `timestamp`,`temperature`,`humidity`,`pressure` FROM `station_%d` ORDER BY `timestamp` ASC LIMIT %d OFFSET %d;" % (int(station), int(limit), int(offset)))
		rows = cursor.fetchall()
		for row in rows : ret.append(DataPoint(float(row[0]), station, float(row[1]), float(row[2]), float(row[3])))
		return ret
		

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
		c.execute("INSERT INTO `stations` (`id`,`name`,`location`,`description`) VALUES (?,?,?,?);", (station.id, station.name, station.location, station.description))
		c.execute("CREATE TABLE IF NOT EXISTS `station_%d` (`timestamp` INT PRIMARY KEY, `temperature` REAL, `humidity` REAL, `pressure` REAL);" % (int(station.id)))
		
	def insertToken(self, token) :
		c = self._con.cursor()
		c.execute("INSERT INTO `tokens` (`token`,`station`) VALUES (?,?);", (token.token, token.station))
		
	def insertDatapoint(self, dp) :
		c = self._con.cursor()
		c.execute("INSERT OR REPLACE INTO `station_%d` (`timestamp`,`temperature`,`humidity`,`pressure`) VALUES (?,?,?,?);" % (int(dp.station)), (dp.timestamp, dp.temperature, dp.humidity, dp.pressure))
		
	def count_datapoints(self, station) :
		ret = []
		cursor = self._con.cursor()
		cursor.execute("SELECT count(*) AS `count` FROM `station_%d`" % (int(station)))
		return int(cursor.fetchone()[0])
		
	def commit(self): self._con.commit()
		
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
	db_hostname, db_database, db_username, db_password = "", "", "", ""
	confirm = True
	
	args = sys.argv[1:]
	for arg in filter(lambda x : x[0] == '-', args) :
		if arg == "-y" or arg == "--yes" :
			confirm = False
			args.remove("-y")
		elif arg == "-h" or arg == "--help" :
			print("meteo Migration utility")
			print("  Migrate your meteo database from MySQL to SQLite3")
			print("")
			print("Usage: %s [HOSTNAME DATABASE USERNAME] [SQLITE3]" % sys.argv[0])
			print("       if [HOSTNAME DATABASE USERNAME] is given, then those values will be used for the mysql connection")
			print("       the password will be still asked")
			print("       if SQLITE3 is given, this will be the filename for the destination database")
			print("")
			print("OPTIONS")
			print("  -h, --help          Print this help message")
			print("  -y, --yes           Don't prompt for confirmation before transferring")
			print("")
			print("2019, Felix Niederwanger")
			sys.exit(0)
		else :
			raise ValueError("Illegal argument: " + arg)
	
	args = list(filter(lambda x : x[0] != '-', args))
	
	if len(sys.argv) > 1 : dbfilename = sys.argv[1]
	if len(sys.argv) > 4 : 
		db_hostname = sys.argv[1]
		db_database = sys.argv[2]
		db_username = sys.argv[3]
		dbfilename = sys.argv[4]
		db_password = getPassword()
	else :
		print("Configuring MySQL Database:")
		db_hostname = input("Hostname> ")
		db_database = input("Database> ")
		db_username = input("Username> ")
		db_password = getPassword()
	if dbfilename is None : 
		print("Configuring sqlite database:")
		dbfilename = input("Filename> ")
	
	# Connect to MySQL
	db, sq = None, None
	try :
		print("Connecting to %s@%s/%s ... " % (db_username, db_hostname, db_database))
		db = MyMeteo(db_hostname, db_username, db_password, db_database)
		print("Database connected : %s " % db.db_version())
		stations = db.stations()
		print("Fetched %d stations:" % (len(stations)))
		datapoints = 0
		for station in stations :
			dps = db.count_datapoints(station.id)
			datapoints += dps
			print("\t%s [%d datapoints]" % (station, dps))
		print("Counter %d datapoints in all stations" % (datapoints))
		tokens = db.tokens()
		print("Fetched %d tokens" % (len(tokens)))
		
		
		while confirm :
			yes = input("Continue? [Y/n] ").lower()
			if yes in ["no", "n"] : sys.exit(1)
			if yes in ["yes", "y"] : break
		
		print("Creating %s ... " % (dbfilename))
		sq = MeteoDB(dbfilename)
		sq.prepare()
		print("\tImporting stations ... ")
		for station in stations : sq.insertStation(station)
		sq.commit()
		print("\tImporting tokens ... ")
		for token in tokens : sq.insertToken(token)
		sq.commit()
		print("\tImporting datapoints ... ")
		bunch = 1000
		for station in stations : 
			count = db.count_datapoints(station.id)
			counter = 0
			sys.stdout.write("\t\t%s - %d datapoints ... " % (station.name, count))
			sys.stdout.flush()
			while counter < count :
				for dp in db.datapoints(station.id, bunch, counter) :
					sq.insertDatapoint(dp)
				counter += bunch
			sq.commit()
			count2 = sq.count_datapoints(station.id)
			if count2 != count : 
				sys.stderr.write("Imported %d datapoints, but source holds %d datapoints\n" % (count2, count))
				sys.exit(1)
			sys.stdout.write("ok (%d records counted)\n" % (count2))
		
		print("Import completed")
		
	finally :
		if not db is None : db.close()
		if not sq is None : sq.close()
	

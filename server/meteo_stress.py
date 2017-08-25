#!/usr/bin/env python3
# -*- coding: utf-8 -*-

link = "localhost:8900"

import sys
import requests


if __name__ == "__main__":
	COUNT=1000
	print("Stressing ... ")
	
	def req(link) :
		r = requests.get(link)
		c = r.status_code
		if c != 200 : raise ValueError("Return value not 200 (" + str(c) + ")")
		return r.text
		
	def stations() :
		html = req("http://" + link + "/nodes?format=plain").split("\n")
		ret = []
		for station in html :
			station = station.strip()
			if len(station) == 0 : continue
			station = [x.strip() for x in station.split(",")]
			_id,name = int(station[0]), station[1]
			ret.append( (_id,name) )
		return ret
		
	
	for i in range(COUNT):
		req("http://" + link + "/current?format=plain")
		for station in stations() : 
			_id,name = station
			r = req("http://" + link + "/node?id=" + str(_id) + "&format=csv")
			req("http://" + link + "/node?id=" + str(_id) + "&format=html")
		print("  Test " + str(i) + "/" + str(COUNT) + " successfull")
	
	print("Bye")

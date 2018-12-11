#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This is a test script, simulating a virtual station for the meteo environmental monitoring systme


import paho.mqtt.client as paho
import random
import sys
import time


if __name__ == "__main__" :
	client = "virtual_station"
	hostname = "localhost"
	port = 1883
	delay = 2 			## publish delay in s
	station_id = 2222
	station_name = "Virtual"
	
	for i in range(1, len(sys.argv)) :
		if sys.argv[i] == "-h" or sys.argv[i] == "--help" :
			print("Virtual station test script")
			print("Usage: " + sys.argv[0] + " [REMOTE]")
			sys.exit(0)
	
	if len(sys.argv) > 1 :
		hostname = sys.argv[1]
	
	client = paho.Client(client)
	client.connect(hostname, port)
	
	t, hum, p = 20, 75, 990
	# Now enter main routine
	def enforceRange(x,x_min,x_max) : return max(x_min, min(x_max, x))
	while True :
		client.loop()
		
		t = enforceRange(t+random.uniform(-0.5, 0.5), 10, 30)
		hum = enforceRange(hum + random.uniform(-0.5, 0.5), 60, 95)
		p = enforceRange(p + random.uniform(-10,10), 900, 1100)
		
		# {"node":5,"name":"Lightning","t":1.95,"hum":82.10,"p":95542.65}
		json = '{"node":%d,"name":"%s","t":%f,"hum":%f,"p":%f}' % (station_id, station_name, t, hum, p)
		print(json)
		client.publish("meteo/%d"%station_id, json)
		time.sleep(delay)

#!/bin/bash

# meteo TEST script
echo "ROOM 1 44 53 22 0"
echo "ROOM 2 42 52 21 0"
echo "NODE 5 temperature=20, humidity=50, light=1000"
echo "NODE 5 temperature=21, humidity=51, light=1200"
echo "list"
sleep 1
echo "ROOM 1 45 52 22 0"
echo "ROOM 2 47 53 22.3 0"
echo "NODE 5 temperature=22, humidity=52, light=1100"
echo "NODE 5 temperature=23, humidity=53, light=1000"
echo "list"
sleep 1
echo "ROOM 1 44 53 22 0"
echo "NODE 5 temperature=21, humidity=55, light=800"
echo "NODE 5 temperature=18, humidity=62, light=900"
echo "list"
sleep 10
echo "ROOM 1 44 53 22 0"
echo "ROOM 2 44 53 22 0"
sleep 2




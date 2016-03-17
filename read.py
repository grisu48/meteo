#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket

# create an INET, STREAMing socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# now connect to the web server on port 80 - the normal http port
s.connect(("localhost", 7443))

def recv(sock) :
	chunks = []
	while True:
		chunk = sock.recv(4096)
		if chunk == b'': break
		chunks.append(chunk)
	return b''.join(chunks)
	
	
try :
	
	msg = recv(s)
	print str(msg)
	
finally:
	s.close()

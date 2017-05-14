#!/bin/bash

openssl genrsa -out certs/serverkeypri.pem 2048
openssl rsa -in certs/serverkeypri.pem -pubout > certs/serverkeypub.pem
openssl req -new -key certs/serverkeypri.pem -out certs/servercsr.pem <<- EOF
	ES
	Madrid
	Madrid
	UAM
	UAM
	G-2302-02-P3-server
	.
	0000
	EPS
EOF
openssl x509 -req -in certs/servercsr.pem -CA certs/ca.pem -CAcreateserial -out certs/servercert.pem
cat certs/servercert.pem certs/serverkeypri.pem certs/rootcert.pem > certs/servidor.pem
openssl x509 -subject -issuer -noout -in certs/servidor.pem

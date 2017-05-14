#!/bin/bash
mkdir -p certs

openssl genrsa -out certs/rootkey.pem 2048
openssl req -new -x509 -key certs/rootkey.pem -out certs/rootcert.pem <<- EOF
	ES
	Madrid
	Madrid
	UAM
	UAM
	Redes2 CA
	.
EOF
cat certs/rootcert.pem certs/rootkey.pem > certs/ca.pem
openssl x509 -subject -issuer -noout -in certs/ca.pem

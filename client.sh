#!/bin/bash

openssl genrsa -out certs/clientkeypri.pem 2048
openssl rsa -in certs/clientkeypri.pem -pubout > certs/clientkeypub.pem
openssl req -new -key certs/clientkeypri.pem -out certs/clientcsr.pem <<- EOF
	ES
	Madrid
	Madrid
	UAM
	UAM
	G-2302-02-P3-client
	.
	0000
	EPS
EOF
openssl x509 -req -in certs/clientcsr.pem -CA certs/ca.pem -CAcreateserial -out certs/clientcert.pem 
cat certs/clientcert.pem certs/clientkeypri.pem certs/rootcert.pem > certs/cliente.pem
openssl x509 -subject -issuer -noout -in certs/cliente.pem

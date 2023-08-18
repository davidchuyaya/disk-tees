#!/bin/bash
# From https://stackoverflow.com/a/10176685/4028758.
# TODO: Change CN to IP address of the server.
openssl req -x509 -newkey rsa:4096 -keyout server_key.pem -out server_cert.pem -sha256 -days 365 -nodes -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Microsoft/OU=ConfidentialComputing/CN=127.0.0.1"
openssl req -x509 -newkey rsa:4096 -keyout client_key.pem -out client_cert.pem -sha256 -days 365 -nodes -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Microsoft/OU=ConfidentialComputing/CN=127.0.0.1"
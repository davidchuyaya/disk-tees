#!/bin/bash
#
#  Create certificate and private key
#
cd /home/azureuser/disk-tees
# Get private IP address
IP=$(hostname -I | xargs)
openssl req -x509 -newkey rsa:4096 -keyout server_key.pem -out server_cert.pem \
  -sha256 -days 365 -nodes -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Microsoft/OU=ConfidentialComputing/CN=$IP"
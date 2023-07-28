#!/bin/bash
# Get private IP address
IP=$(hostname -I | xargs)
# Create certificate and private key
openssl req -x509 -newkey rsa:4096 -keyout /home/azureuser/server_key.pem -out /home/azureuser/server_cert.pem \
  -sha256 -days 365 -nodes -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Microsoft/OU=ConfidentialComputing/CN=$IP"
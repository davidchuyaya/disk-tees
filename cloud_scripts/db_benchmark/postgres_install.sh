#!/bin/bash
#
# Downloading and building postgres
#
cd /home/azureuser
sudo apt update
sudo apt -y install build-essential libreadline-dev zlib1g-dev
# Postgres 15.4 is the latest non-beta at the time of writing: https://www.postgresql.org/ftp/source/
wget https://ftp.postgresql.org/pub/source/v15.4/postgresql-15.4.tar.gz
tar -xf postgresql-15.4.tar.gz
cd postgresql-15.4/
./configure
make
sudo make install
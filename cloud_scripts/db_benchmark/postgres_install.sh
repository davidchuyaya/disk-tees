#!/bin/bash
#
# Downloading and building postgres
#
USERNAME=$(whoami)
BUILD_DIR=/home/$USERNAME/disk-tees/build
mkdir -p $BUILD_DIR
cd $BUILD_DIR
# Postgres 15.4 is the latest non-beta at the time of writing: https://www.postgresql.org/ftp/source/
if [ -d postgresql-15.4 ]; then
  echo "Postgres already installed"
  exit 0 
fi

echo "Will need sudo access to install postgres and its dependencies"
sudo apt update
sudo apt -y install build-essential libreadline-dev zlib1g-dev

wget https://ftp.postgresql.org/pub/source/v15.4/postgresql-15.4.tar.gz
tar -xf postgresql-15.4.tar.gz
rm postgresql-15.4.tar.gz
cd postgresql-15.4/
./configure
make
sudo make install
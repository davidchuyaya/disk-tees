#!/bin/bash
#
# Downloading and building postgres
#
print_usage() {
    echo "Usage: $0 -t <trusted mode>"
}

while getopts 't:' flag; do
  case ${flag} in
    t) TRUSTED_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

if [ $TRUSTED_MODE == "local" ]; then
    HOME_DIR=~
else
    HOME_DIR=/home/azureuser
fi

BUILD_DIR=$HOME_DIR/disk-tees/build
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
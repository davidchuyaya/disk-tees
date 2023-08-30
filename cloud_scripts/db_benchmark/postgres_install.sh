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


sudo apt update
sudo apt -y install build-essential libreadline-dev zlib1g-dev

$BUILD_DIR=$HOME_DIR/disk-tees/build
mkdir -p $BUILD_DIR
cd $BUILD_DIR
# Postgres 15.4 is the latest non-beta at the time of writing: https://www.postgresql.org/ftp/source/
wget https://ftp.postgresql.org/pub/source/v15.4/postgresql-15.4.tar.gz
tar -xf postgresql-15.4.tar.gz
cd postgresql-15.4/
./configure
make
sudo make install
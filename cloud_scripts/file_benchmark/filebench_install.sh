#!/bin/bash
#
# Setting up filebench, according to https://github.com/filebench/filebench/wiki#quick-start-guide
#
print_usage() {
    echo "Usage: $0 -u <username>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'u:' flag; do
  case ${flag} in
    u) USERNAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

cd /home/$USERNAME
wget https://github.com/filebench/filebench/releases/download/1.5-alpha3/filebench-1.5-alpha3.tar.gz
tar -xvf filebench-1.5-alpha3.tar.gz
rm filebench-1.5-alpha3.tar.gz
cd filebench-1.5-alpha3
# Install dependencies: https://github.com/filebench/filebench/wiki/Building-Filebench#building-filebench-from-the-git-repository
sudo apt update
sudo apt -y install bison flex build-essential
./configure
make
sudo make install
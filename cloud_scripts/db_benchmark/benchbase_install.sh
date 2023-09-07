#!/bin/bash
#
# Cloning and building benchbase, according to https://github.com/cmu-db/benchbase/blob/main/README.md
#
print_usage() {
    echo "Usage: $0  -u <username>"
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
git clone --depth 1 https://github.com/cmu-db/benchbase.git
cd benchbase
# Install Java
sudo apt update
sudo apt -y install openjdk-17-jdk
./mvnw clean package -P postgres -DskipTests
cd target
tar xvzf benchbase-postgres.tgz
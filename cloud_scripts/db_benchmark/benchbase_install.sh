#!/bin/bash
#
# Cloning and building benchbase, according to https://github.com/cmu-db/benchbase/blob/main/README.md
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
    cd ~
else
    cd /home/azureuser
fi

git clone --depth 1 https://github.com/cmu-db/benchbase.git
cd benchbase
# Install Java
sudo apt update
sudo apt -y install openjdk-17-jdk
./mvnw clean package -P postgres -DskipTests
cd target
tar xvzf benchbase-postgres.tgz
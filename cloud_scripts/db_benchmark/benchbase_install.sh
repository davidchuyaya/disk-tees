#!/bin/bash
#
# Cloning and building benchbase, according to https://github.com/cmu-db/benchbase/blob/main/README.md
#
USERNAME=$(whoami)
cd /home/$USERNAME
git clone --depth 1 https://github.com/cmu-db/benchbase.git
cd benchbase
# Install Java
sudo apt update
sudo apt -y install openjdk-17-jdk
./mvnw clean package -P postgres -DskipTests
cd target
tar xvzf benchbase-postgres.tgz
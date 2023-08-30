#!/bin/bash

# Setting up client
mkdir ~/mountDir
mkdir ~/redirectDir
sudo mount -t tmpfs -o size=2G tmpfs ~/redirectDir
client/tee_fuse -i 1 -r ~/redirectDir -f -s ~/mountDir

# Setting up replica
mkdir ~/replicaDir
sudo mount -t tmpfs -o size=2G tmpfs ~/replicaDir
replica/disk_tees -i 0 -d ~/replicaDir

# Running postgres
mkdir ~/mountDir/data
/usr/local/pgsql/bin/initdb -D ~/mountDir/data
/usr/local/pgsql/bin/pg_ctl -D ~/mountDir/data -l logfile start
/usr/local/pgsql/bin/createuser -s -i -d -r -l -w admin
/usr/local/pgsql/bin/createdb benchbase
cd ~/benchbase/target/benchbase-postgres
java -jar benchbase.jar -b tpcc -c config/postgres/sample_tpcc_config.xml --create=true --load=true --execute=true

# Cleanup
sudo umount ~/redirectDir
sudo umount ~/replicaDir
rm -rf ~/mountDir
rm -rf ~/redirectDir
rm -rf ~/replicaDir
/usr/local/pgsql/bin/pg_ctl -D ~/mountDir/data stop
#!/bin/bash
# 
# Run the replicas' code.
#
cd /home/azureuser
git clone https://github.com/davidchuyaya/disk-tees.git
cd /home/azureuser/disk-tees

# Create certificates
cloud_scripts/create_cert.sh
# TODO: Communicate certificates to all other replicas & client

# Mount tmpfs for mirroring client writes
DIR="/home/azureuser/postgres_tmpfs"
mkdir $DIR
# TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $DIR

# Make disk-tees, mount it on $DIR
cmake .
make
# TODO: Run in background so startup script can terminate?
ID=0 # TODO: Assign id
replica/disk-tees -i $ID -d $DIR
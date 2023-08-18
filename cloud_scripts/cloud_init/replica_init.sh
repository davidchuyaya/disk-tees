#!/bin/bash
# 
# Run the replicas' code.
#
git clone https://github.com/davidchuyaya/disk-tees.git /home/azureuser/disk-tees
cd /home/azureuser/disk-tees

# Create certificates
cloud_scripts/create_cert.sh
# TODO: Communicate certificates to all other replicas & client

# Mount tmpfs for mirroring client writes
DIR="/home/azureuser/postgres_tmpfs"
mkdir $DIR
# TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $DIR

# TODO: Make disk-tees, mount it on $DIR
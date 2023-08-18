#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs
#
git clone https://github.com/davidchuyaya/disk-tees.git /home/azureuser/disk-tees
cd /home/azureuser/disk-tees
cloud_scripts/db_benchmark/postgres_install.sh
DIR="/home/azureuser/postgres_fuse"
mkdir $DIR
#TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $DIR
#TODO: Make tee_fuse, mount it on $DIR
# Create subdirectory for postgres. Necessary because postgres wants to modify the permissions(?) and can't do it if that's where tmpfs is directly mounted
SUBDIR=${DIR}/data
mkdir $SUBDIR
cloud_scripts/db_benchmark/postgres_run.sh -d $SUBDIR
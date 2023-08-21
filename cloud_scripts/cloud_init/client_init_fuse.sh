#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs, redirected through FUSE
#
cd /home/azureuser
git clone https://github.com/davidchuyaya/disk-tees.git
cd /home/azureuser/disk-tees
cloud_scripts/db_benchmark/postgres_install.sh

# Create directory to store files
TMPFS_DIR="/home/azureuser/postgres_storage"
mkdir $TMPFS_DIR
#TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $TMPFS_DIR
sudo chmod 777 $TMPFS_DIR

# Create directory that postgres interfaces with, where operations are intercepted and redirected
DIR="/home/azureuser/postgres_fuse"
mkdir $DIR

# Make tee_fuse, mount it on $DIR and redirect to $TMPFS_DIR
ID=0
cmake .
make
client/tee_fuse -i $ID -r $TMPFS_DIR -f -s $DIR # TODO: Turn off network flag?

# Create subdirectory for postgres. Necessary because postgres wants to modify the permissions(?) and can't do it if that's where tmpfs is directly mounted
SUB_DIR=${DIR}/data
mkdir $SUB_DIR
cloud_scripts/db_benchmark/postgres_run.sh -d $SUB_DIR
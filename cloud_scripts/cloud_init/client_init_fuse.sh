#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs, redirected through FUSE
#
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID
#   TMPFS_MEMORY: how much memory to allocate to tmpfs (in Gb)

if [ $TRUSTED_MODE == "local" ]; then
    HOME_DIR=~
    # Assume that disk-tees has been cloned if this is executing locally
else
    HOME_DIR=/home/azureuser
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=client${ID}
BUILD_DIR=$HOME_DIR/disk-tees/build/$NAME
mkdir -p $BUILD_DIR

# Create directory to store files
TMPFS_DIR=${BUILD_DIR}/storage
mkdir -p $TMPFS_DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $TMPFS_DIR

# Create directory that postgres interfaces with, where operations are intercepted and redirected
DIR=${BUILD_DIR}/shim
mkdir -p $DIR

# Make tee_fuse and mount it on $DIR
cd $HOME_DIR/disk-tees
cmake .
make
cd $BUILD_DIR
$HOME_DIR/disk-tees/client/tee_fuse -i $ID -t $TRUSTED_MODE -s $DIR
$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_install.sh -t $TRUSTED_MODE
$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
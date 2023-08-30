#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs, redirected through FUSE
#
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"

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
mkdir $TMPFS_DIR
#TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $TMPFS_DIR

# Create directory that postgres interfaces with, where operations are intercepted and redirected
DIR=${BUILD_DIR}/shim
mkdir $DIR

# Make tee_fuse, mount it on $DIR and redirect to $TMPFS_DIR
INSTALL_SCRIPT="$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_install.sh -t $TRUSTED_MODE"
RUN_SCRIPT="$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR"
cd $HOME_DIR/disk-tees
cmake .
make
cd $BUILD_DIR
$HOME_DIR/client/tee_fuse -i $ID -t $TRUSTED_MODE --install=$INSTALL_SCRIPT --run=$RUN_SCRIPT -f -s $DIR
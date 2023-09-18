#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs, redirected through FUSE
#
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID
#   TMPFS_MEMORY: how much memory to allocate to tmpfs (in Gb)
#   USERNAME: username of the machine that launched these scripts

PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
    cd $PROJECT_DIR
    ./install.sh
fi

NAME=client${ID}
BUILD_DIR=$PROJECT_DIR/build/$NAME
mkdir -p $BUILD_DIR

# Create directory to store files
TMPFS_DIR=${BUILD_DIR}/storage
mkdir -p $TMPFS_DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $TMPFS_DIR

# Create directory that postgres interfaces with, where operations are intercepted and redirected
DIR=${BUILD_DIR}/shim
mkdir -p $DIR

# Make tee_fuse and mount it on $DIR
cd $PROJECT_DIR
cmake .
make
cd $BUILD_DIR
$PROJECT_DIR/client/tee_fuse -i $ID -t $TRUSTED_MODE -u $USERNAME $DIR
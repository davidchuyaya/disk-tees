#!/bin/bash
# 
# Install and run postgres on a single machine.
#
# Assumes the following variables will be inserted via attach_var.sh:
#   ID: client ID
#   USERNAME: username of the machine that launched these scripts

PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=client${ID}
BUILD_DIR=$PROJECT_DIR/build/$NAME
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Mount managed disk. See https://learn.microsoft.com/en-us/azure/virtual-machines/linux/add-disk?tabs=ubuntu#format-the-disk
sudo parted /dev/sdb --script mklabel gpt mkpart ext4 0% 100%
sudo partprobe /dev/sdb
sudo mkfs.ext4 /dev/sdb1

DIR=${BUILD_DIR}/shim
mkdir -p $DIR

sudo mount /dev/sdb1 $DIR
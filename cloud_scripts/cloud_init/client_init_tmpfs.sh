#!/bin/bash
# 
# Install and run postgres on a single machine, writing to tmpfs
#
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID
#   TMPFS_MEMORY: how much memory to allocate to tmpfs (in Gb)

USERNAME=$(whoami)
PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=client${ID}
BUILD_DIR=$PROJECT_DIR/build/$NAME
mkdir -p $BUILD_DIR

$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_install.sh
DIR=$BUILD_DIR/shim
mkdir -p $DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $DIR
$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
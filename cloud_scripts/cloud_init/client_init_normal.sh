#!/bin/bash
# 
# Install and run postgres on a single machine.
#
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID

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
cd $BUILD_DIR

$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_install.sh -t $TRUSTED_MODE
DIR=${BUILD_DIR}/shim
mkdir $DIR
$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
#!/bin/bash
# 
# Install and run postgres on a single machine.
#
# Assumes the following variables will be inserted via attach_var.sh:
#   ID: client ID

USERNAME=$(whoami)
PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=client${ID}
BUILD_DIR=$PROJECT_DIR/build/$NAME
mkdir -p $BUILD_DIR
cd $BUILD_DIR

$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_install.sh
DIR=${BUILD_DIR}/shim
mkdir -p $DIR
$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
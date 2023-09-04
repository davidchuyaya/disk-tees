#!/bin/bash
# 
# Run the replicas' code.
#
# Assumes thae following files will be inserted via attach_file.sh:
#   ccf.json
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: replica ID
#   TMPFS_MEMORY: how much memory to allocate to tmpfs (in Gb)

if [ $TRUSTED_MODE == "local" ]; then
    HOME_DIR=~
    # Assume that disk-tees has been cloned if this is executing locally
else
    HOME_DIR=/home/azureuser
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=replica${ID}
$BUILD_DIR=$HOME_DIR/disk-tees/build/$NAME
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Create certificates and send them to CCF
$HOME_DIR/disk-tees/cloud_scripts/create_cert.sh -t $TRUSTED_MODE -n $NAME
CCF_ADDR=$(jq '.[0].ip' ${BUILD_DIR}/ccf.json)
$HOME_DIR/disk-tees/cloud_scripts/ccf_networking/add_cert.sh -a $CCF_ADDR -i $ID -n $NAME -t "replica"
# Download the client's certificate. Assume that the client's ID is 0.
$HOME_DIR/disk-tees/cloud_scripts/ccf_networking/get_cert.sh -a $CCF_ADDR -i 0 -n "client0" -t "client"

# Mount tmpfs for mirroring client writes
DIR=$BUILD_DIR/storage
mkdir -p $DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $DIR

# Create a separate tmpfs directory, so we have a place to put the zipped storage directory when creating/sending it
ZIPPED_DIR=$BUILD_DIR/zipped-storage
mkdir -p $ZIPPED_DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $ZIPPED_DIR

# Make disk-tees, mount it on $DIR
cd $HOME_DIR/disk-tees
cmake .
make
cd $BUILD_DIR
# Run in background so startup script can terminate
$HOME_DIR/disk-tees/replica/disk-tees -i $ID -t $TRUSTED_MODE &
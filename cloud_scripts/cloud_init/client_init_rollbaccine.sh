#!/bin/bash
# 
# Install and run postgres, writing to tmpfs, forwarding writes to all replicas
#
# Assumes thae following files will be inserted via attach_file.sh:
#   ccf.json
#   replicas.json
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID
#   TMPFS_MEMORY: how much memory to allocate to tmpfs (in Gb)
#   WAIT_SECS: how long to wait between starting the client and starting postgres

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

# Create certificates and send them to CCF
cd $BUILD_DIR
$HOME_DIR/disk-tees/cloud_scripts/create_cert.sh -t $TRUSTED_MODE -n $NAME
CCF_ADDR=$(jq '.[0].ip' ${BUILD_DIR}/ccf.json)
$HOME_DIR/disk-tees/cloud_scripts/ccf_networking/add_cert.sh -a $CCF_ADDR -i $ID -n $NAME -t "client"
# Download the replicas' certificates
REPLICA_IDS=$(jq '.[].id' ${BUILD_DIR}/replicas.json)
for REPLICA_ID in "${REPLICA_IDS[@]}"
do
    $HOME_DIR/disk-tees/cloud_scripts/ccf_networking/get_cert.sh -a $CCF_ADDR -i $REPLICA_ID -n "replica${REPLICA_ID}" -t "replica"
done

# Create directory to store files
TMPFS_DIR=${BUILD_DIR}/storage
mkdir -p $TMPFS_DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $TMPFS_DIR

# Create a separate tmpfs directory, so we have a place to put the zipped storage directory when creating/sending it
ZIPPED_DIR=$BUILD_DIR/zipped-storage
mkdir -p $ZIPPED_DIR
sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $ZIPPED_DIR

# Create directory that postgres interfaces with, where operations are intercepted and redirected
DIR=${BUILD_DIR}/shim
mkdir -p $DIR

# Start background process to make sure FUSE checks for messages periodically
$HOME_DIR/disk-tees/cloud_scripts/fuse_waker.sh -n $NAME -t $TRUSTED_MODE &

# Make tee_fuse and mount it on $DIR
cd $HOME_DIR/disk-tees
cmake .
make
cd $BUILD_DIR
$HOME_DIR/disk-tees/client/tee_fuse -i $ID -t $TRUSTED_MODE -n -f -s $DIR
sleep $WAIT_SECS
$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_install.sh -t $TRUSTED_MODE
$HOME_DIR/disk-tees/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
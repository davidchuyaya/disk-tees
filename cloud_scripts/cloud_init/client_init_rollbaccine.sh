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
mkdir $TMPFS_DIR
#TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $TMPFS_DIR

# Create a separate tmpfs directory, so we have a place to put the zipped storage directory when creating/sending it
ZIPPED_DIR=$BUILD_DIR/zipped-storage
mkdir $ZIPPED_DIR
# TODO: Figure out how large to make tmpfs (how many Gb)
sudo mount -t tmpfs -o size=2G tmpfs $ZIPPED_DIR

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
$HOME_DIR/disk-tees/client/tee_fuse -i $ID -t $TRUSTED_MODE -n --install=$INSTALL_SCRIPT --run=$RUN_SCRIPT -f -s $DIR
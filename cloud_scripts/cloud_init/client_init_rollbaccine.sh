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

USERNAME=$(whoami)
PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

NAME=client${ID}
BUILD_DIR=$PROJECT_DIR/build/$NAME
mkdir -p $BUILD_DIR

# Create certificates and send them to CCF
cd $BUILD_DIR
$PROJECT_DIR/cloud_scripts/create_cert.sh -t $TRUSTED_MODE -n $NAME
CCF_ADDR=$(jq -r '.[0].ip' ${BUILD_DIR}/ccf.json)
$PROJECT_DIR/cloud_scripts/ccf_networking/add_cert.sh -a $CCF_ADDR -i $ID -n $NAME -t "client"
# Download the replicas' certificates. See https://stackoverflow.com/a/49302719/4028758 for reading JSON into bash array
readarray -t REPLICA_IDS < <(jq -r -c '.[].id' replicas.json)
for REPLICA_ID in "${REPLICA_IDS[@]}"
do
    $PROJECT_DIR/cloud_scripts/ccf_networking/get_cert.sh -a $CCF_ADDR -i $REPLICA_ID -n "replica${REPLICA_ID}" -t "replica"
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
$PROJECT_DIR/cloud_scripts/fuse_waker.sh -n $NAME &

# Make tee_fuse and mount it on $DIR
cd $PROJECT_DIR
cmake .
make
cd $BUILD_DIR
# tee_fuse has issues if it's run too early after cmake?
sleep 1
$PROJECT_DIR/client/tee_fuse -i $ID -t $TRUSTED_MODE -n -f -s $DIR > $BUILD_DIR/log.txt 2>&1 &
# Install and run postgres
sleep $WAIT_SECS
$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_install.sh
$PROJECT_DIR/cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
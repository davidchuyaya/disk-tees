#!/bin/bash
print_usage() {
    echo "Usage: $0 -n <name> -t <trusted mode> -c <checksum>"
    echo "Unzip disk if it matches the checksum"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'n:t:c:' flag; do
  case ${flag} in
    n) NAME=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    c) CHECKSUM=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

if [ $TRUSTED_MODE == "local" ]; then
    BUILD_DIR=~/disk-tees/build/$NAME
else
    BUILD_DIR=/home/azureuser/disk-tees/build/$NAME
fi
SRC_FILE=$BUILD_DIR/zipped-storage/storage.tar.gz
OUTPUT_DIR=$BUILD_DIR/storage

# Wait for file to arrive
while [ ! -f $SRC_FILE ]; do
    echo "Waiting for storage.tar.gz to arrive, checking again in 5s..."
    sleep 5
done

# Check checksum
MD5=($(md5sum $SRC_FILE))
if [ $CHECKSUM != $MD5 ]; then
    echo "Checksums do not match"
else
    # Clear destination
    rm -rf $OUTPUT_DIR/*
    # Unzip
    tar -zxf $SRC_FILE --directory $OUTPUT_DIR
fi
# Cleanup
rm -rf $BUILD_DIR/zipped-storage/*
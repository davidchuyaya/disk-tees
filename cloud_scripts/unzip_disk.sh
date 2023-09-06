#!/bin/bash
print_usage() {
    echo "Usage: $0 -s <source name> -d <destination name> -c <checksum>"
    echo "Unzip disk if it matches the checksum"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 's:d:c:' flag; do
  case ${flag} in
    s) SRC_NAME=${OPTARG} ;;
    d) DST_NAME=${OPTARG} ;;
    c) CHECKSUM=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

USERNAME=$(whoami)
BUILD_DIR=/home/$USERNAME/disk-tees/build/$DST_NAME
SRC_FILE=$BUILD_DIR/zipped-storage/$SRC_NAME.tar.gz
OUTPUT_DIR=$BUILD_DIR/storage

# Wait for file to arrive
while [ ! -f $SRC_FILE ]; do
    echo "Waiting for $SRC_NAME.tar.gz to arrive, checking again in 5s..."
    sleep 5
done

# Check checksum
MD5=($(md5sum $SRC_FILE))
if [ $CHECKSUM != $MD5 ]; then
    echo "Checksums do not match"
    exit 1
else
    # Clear destination
    rm -rf $OUTPUT_DIR/*
    # Unzip
    tar -zxf $SRC_FILE --directory $OUTPUT_DIR
fi
# Cleanup
# rm -rf $BUILD_DIR/zipped-storage/*
exit 0
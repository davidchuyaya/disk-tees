#!/bin/bash
print_usage() {
    echo "Usage: $0 -n <name> -t <trusted mode>"
    echo "Periodically calls 'ls' on the FUSE directory so it can poll for messages (for reconfiguration)"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'n:t:' flag; do
  case ${flag} in
    n) NAME=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

if [ $TRUSTED_MODE == "local" ]; then
    BUILD_DIR=~/disk-tees/build/$NAME
else
    BUILD_DIR=/home/azureuser/disk-tees/build/$NAME
fi
SRC_DIR=$BUILD_DIR/shim

while true; do
    sleep 5
    ls $SRC_DIR
    echo "Woke FUSE, sleeping..."
done
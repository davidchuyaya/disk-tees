#!/bin/bash
print_usage() {
    echo "Usage: $0 -n <name>"
    echo "Periodically calls 'ls' on the FUSE directory so it can poll for messages (for reconfiguration)"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'n:' flag; do
  case ${flag} in
    n) NAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

USERNAME=$(whoami)
BUILD_DIR=/home/$USERNAME/disk-tees/build/$NAME
SRC_DIR=$BUILD_DIR/shim

while true; do
    sleep 5
    ls $SRC_DIR
    echo "Woke FUSE, sleeping..."
done
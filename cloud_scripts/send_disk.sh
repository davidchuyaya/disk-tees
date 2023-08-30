#!/bin/bash
print_usage() {
    echo "Usage: $0 -s <source name> -d <destination name> -a <destination ip:port> -t <trusted mode>"
    echo "Copies the storage directory from source to dest and stores its md5 checksum"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 's:d:a:t:' flag; do
  case ${flag} in
    s) SRC_NAME=${OPTARG} ;;
    d) DST_NAME=${OPTARG} ;;
    a) ADDR=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

if [ $TRUSTED_MODE == "local" ]; then
    BUILD_DIR=~/disk-tees/build/$SRC_NAME
else
    BUILD_DIR=/home/azureuser/disk-tees/build/$SRC_NAME
fi
SRC_DIR=$BUILD_DIR/storage
OUTPUT_FILE=$BUILD_DIR/zipped-storage/storage.tar.gz

# Zip
tar -zcf $OUTPUT_FILE -C $SRC_DIR .
# Output checksum
# TODO: Delete this file after reading
MD5=($(md5sum $OUTPUT_FILE))
echo $MD5 > $OUTPUT_FILE.md5
# Send
if [ $TRUSTED_MODE == "local" ]; then
    cp $OUTPUT_FILE ~/disk-tees/build/$DST_NAME/zipped-storage
else
    scp $OUTPUT_FILE azureuser@$ADDR:/home/azureuser/disk-tees/build/$DST_NAME/zipped-storage
fi
rm $OUTPUT_FILE
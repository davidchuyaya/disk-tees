#!/bin/bash
print_usage() {
    echo "Usage: $0 -s <source name> -d <destination name> -a <destination ip:port> -t <trusted mode> [-z]"
    echo "Copies the storage directory from source to dest and stores its md5 checksum. If -z is not specified, does not zip or create checksum."
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 's:d:a:t:z' flag; do
  case ${flag} in
    s) SRC_NAME=${OPTARG} ;;
    d) DST_NAME=${OPTARG} ;;
    a) ADDR=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    z) ZIP=true ;;
    *) print_usage
       exit 1;;
  esac
done

USERNAME=$(whoami)
BUILD_DIR=/home/$USERNAME/disk-tees/build/$SRC_NAME
SRC_DIR=$BUILD_DIR/storage
OUTPUT_FILE=$BUILD_DIR/zipped-storage/$SRC_NAME.tar.gz

if [ $ZIP == true ]; then
    # Zip
    tar -zcf $OUTPUT_FILE -C $SRC_DIR .
    # Output checksum
    MD5=($(md5sum $OUTPUT_FILE))
    echo $MD5 > $OUTPUT_FILE.md5
fi
# Send
if [ $TRUSTED_MODE == "local" ]; then
    cp $OUTPUT_FILE ~/disk-tees/build/$DST_NAME/zipped-storage
else
    scp $OUTPUT_FILE $USERNAME@$ADDR:/home/$USERNAME/disk-tees/build/$DST_NAME/zipped-storage
fi
# rm $OUTPUT_FILE
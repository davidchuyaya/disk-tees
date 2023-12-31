#!/bin/bash
#
# Setting up filebench, according to https://github.com/filebench/filebench/wiki#quick-start-guide
#
print_usage() {
    echo "Usage: $0 -u <username> -f <file system mode> [-m <tmpfs memory size (GB)> -t <trusted mode>]"
    echo "-m only relevant for tmpfs, fuse, rollbaccine (the modes that use tmpfs)"
    echo "-t only relevant for fuse, rollbaccine (to pass as a variable to tee_fuse)"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'u:f:m:t:' flag; do
  case ${flag} in
    u) USERNAME=${OPTARG} ;;
    f) FILE_SYSTEM_MODE=${OPTARG} ;;
    m) TMPFS_MEMORY=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

PROJECT_DIR=/home/$USERNAME/disk-tees
cd $PROJECT_DIR/cloud_scripts/file_benchmark
BUILD_DIR=$PROJECT_DIR/build/client0

CONFIGS=("seq-read-1th-64g.f" "seq-read-32th-64g.f" "seq-write-1th-64g.f" "seq-write-32th-64g.f" "seq-fsync-1th-64g.f" "seq-fsync-32th-64g.f" "rnd-read-32th-64g.f" "rnd-write-32th-64g.f" "rnd-fsync-32th-64g.f" "file-server.f" "mail-server.f" "web-server.f")
DIR=$BUILD_DIR/shim

RESULTS_DIR=$BUILD_DIR/results
mkdir -p $RESULTS_DIR
# See issue: https://github.com/filebench/filebench/issues/112
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

run_test() {
  # Set mount directory
  sed "1i set "'$dir='"$DIR" $1 > $RESULTS_DIR/$1
  # See output as it runs and pipe to log
  sudo filebench -f $RESULTS_DIR/$1 |& tee $RESULTS_DIR/$1.log
}

case $FILE_SYSTEM_MODE in
  "normal")
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        run_test $CONFIG
        sudo umount $DIR
        sudo mkfs.ext4 -F /dev/sdb1
        sudo mount /dev/sdb1 $DIR
    done;;
  "tmpfs")
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        run_test $CONFIG
        sudo umount $DIR
        sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $DIR
    done;;
  "fuse")
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        run_test $CONFIG
        pgrep -f tee_fuse | xargs kill -9
        sudo umount $DIR
        sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $BUILD_DIR/storage
        $PROJECT_DIR/client/tee_fuse -i 0 -t $TRUSTED_MODE -u $USERNAME -o allow_other $DIR
    done;;
  "rollbaccine")
    echo "Unimplemented. Need to tell replicas to clear directory, then restart client without contacting CCF";;
esac
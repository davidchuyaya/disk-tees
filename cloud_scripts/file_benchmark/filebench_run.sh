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
    # Mount managed disk. See https://learn.microsoft.com/en-us/azure/virtual-machines/linux/add-disk?tabs=ubuntu#format-the-disk
    sudo parted /dev/sda --script mklabel gpt mkpart ext4 0% 100%
    sudo partprobe /dev/sda
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        sudo mkfs.ext4 /dev/sda1 # Reformat and remount on each test
        sudo mount /dev/sda1 $DIR
        run_test $CONFIG
        sudo umount -l $DIR
    done;;
  "tmpfs")
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $DIR
        run_test $CONFIG
        sudo umount -l $DIR
    done;;
  "fuse")
    for CONFIG in "${CONFIGS[@]}"; do
        echo "Running $CONFIG..."
        sudo mount -t tmpfs -o size=${TMPFS_MEMORY}G tmpfs $BUILD_DIR/storage
        run_test $CONFIG
        sudo umount -l $DIR
        pgrep -f tee_fuse | xargs kill -9
        $PROJECT_DIR/client/tee_fuse -i 0 -t $TRUSTED_MODE -u $USERNAME -s $DIR
    done;;
  "rollbaccine")
    echo "Unimplemented. Need to tell replicas to clear directory, then restart client without contacting CCF";;
esac
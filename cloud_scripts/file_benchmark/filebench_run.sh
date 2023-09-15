#!/bin/bash
#
# Setting up filebench, according to https://github.com/filebench/filebench/wiki#quick-start-guide
#
print_usage() {
    echo "Usage: $0 -u <username>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'u:' flag; do
  case ${flag} in
    u) USERNAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

PROJECT_DIR=/home/$USERNAME/disk-tees
cd $PROJECT_DIR/cloud_scripts/file_benchmark

CONFIGS=("seq-read-1th-16g.f" "seq-read-32th-16g.f" "seq-write-1th-16g.f" "seq-write-32th-16g.f" "seq-fsync-1th-16g.f" "seq-fsync-32th-16g.f" "seq-read-32th-64g.f" "rnd-read-32th-64g.f" "seq-write-32th-64g.f" "rnd-write-32th-64g.f" "seq-fsync-32th-64g.f" "rnd-fsync-32th-64g.f" "file-server.f" "mail-server.f" "web-server.f")
DIR=$PROJECT_DIR/build/client0/shim

RESULTS_DIR=$PROJECT_DIR/build/client0/results
mkdir -p $RESULTS_DIR
for CONFIG in "${CONFIGS[@]}"; do
    echo "Running $CONFIG..."
    # Set mount directory
    sed "'i $dir='$DIR" $CONFIG > $RESULTS_DIR/$CONFIG
    # See output as it runs and pipe to log
    filebench -f $CONFIG | tee $RESULTS_DIR/$CONFIG.log
done
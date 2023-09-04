#!/bin/bash
echo "Cleaning up, most error messages should be ignored..."

# Kill postgres and CCF
POSTGRES_PIDS=$(pgrep -f postgres)
for POSTGRES_PID in "${POSTGRES_PIDS[@]}"
do
    kill -9 $POSTGRES_PID
done
CCF_PIDS=$(pgrep -f ccf)
for CCF_PID in "${CCF_PIDS[@]}"
do
    kill -9 $CCF_PID
done

# Assumes that there is only 1 client
/usr/local/pgsql/bin/pg_ctl -D ~/disk-tees/build/client0/shim/data stop

# Unmount tmpfs
sudo umount ~/disk-tees/build/client0/shim
sudo umount ~/disk-tees/build/client0/storage
sudo umount ~/disk-tees/build/client0/zipped-storage
# TODO: Unmount tmpfs for replicas

# Remove error for using parentheses in glob patterns. See: https://stackoverflow.com/questions/47304329
shopt -s extglob
# Delete directories
rm -rf ~/disk-tees/build/client0
rm -rf ~/disk-tees/build/replica+([0-9])
rm -rf ~/disk-tees/build/workspace
rm -f ~/disk-tees/build/ccf.json
rm -f ~/disk-tees/build/replicas.json
rm -f ~/disk-tees/build/ccf.log
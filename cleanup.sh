#!/bin/bash
echo "Cleaning up, most error messages should be ignored..."
echo "If a directory fails to unmount because it is busy, manually kill the process that is using it, then unmount it."
echo "May need to rerun twice."
echo ""

# Kill all
pgrep -f disk_tees | xargs kill -9
pgrep -f tee_fuse | xargs kill -9
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

# Kill client's FUSE waker
pgrep -f fuse_waker | xargs kill -9

# Unmount tmpfs for client
sudo umount -l ~/disk-tees/build/client0/shim
sudo umount -l ~/disk-tees/build/client0/storage
sudo umount -l ~/disk-tees/build/client0/zipped-storage
# Unmount tmpfs for replicas
REPLICA_ID=0
while [ -d ~/disk-tees/build/replica$REPLICA_ID ]
do
    sudo umount -l ~/disk-tees/build/replica$REPLICA_ID/shim
    sudo umount -l ~/disk-tees/build/replica$REPLICA_ID/storage
    sudo umount -l ~/disk-tees/build/replica$REPLICA_ID/zipped-storage
    REPLICA_ID=$((REPLICA_ID+1))
done

# Remove error for using parentheses in glob patterns. See: https://stackoverflow.com/questions/47304329
shopt -s extglob
# Delete directories
rm -rf ~/disk-tees/build/client0
rm -rf ~/disk-tees/build/replica+([0-9])
rm -rf ~/disk-tees/build/workspace
rm -f ~/disk-tees/build/ccf.json
rm -f ~/disk-tees/build/replicas.json
rm -f ~/disk-tees/build/ccf.log
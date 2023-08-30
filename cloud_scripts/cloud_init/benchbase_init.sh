#!/bin/bash
# 
# Install and run benchbase.
#
# Assumes thae following files will be inserted via attach_file.sh:
#   ccf.json
# Assumes the following variables will be inserted via attach_var.sh:
#   TRUSTED_MODE: "local", "trusted", or "untrusted"
#   ID: client ID
#   CLIENT_IP: IP address of client

if [ $TRUSTED_MODE == "local" ]; then
    HOME_DIR=~
    # Assume that disk-tees has been cloned if this is executing locally
else
    HOME_DIR=/home/azureuser
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

cd ${HOME_DIR}/disk-tees
./install.sh
cloud_scripts/db_benchmark/benchbase_install.sh -t $TRUSTED_MODE
cd /home/azureuser/disk-tees
cloud_scripts/db_benchmark/benchbase_run.sh -i $CLIENT_IP -t $TRUSTED_MODE
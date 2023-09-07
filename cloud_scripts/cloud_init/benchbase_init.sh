#!/bin/bash
# 
# Install and run benchbase.
#
# Assumes the following variables will be inserted via attach_var.sh:
#   CLIENT_IP: private IP of the client
#   USERNAME: username of the machine that launched these scripts

PROJECT_DIR=/home/$USERNAME/disk-tees
if [ ! -d $PROJECT_DIR ]; then
    cd /home/$USERNAME
    git clone https://github.com/davidchuyaya/disk-tees.git
fi

cloud_scripts/db_benchmark/benchbase_install.sh -u $USERNAME
cd ${HOME_DIR}/disk-tees
cloud_scripts/db_benchmark/benchbase_run.sh -i $CLIENT_IP -u $USERNAME
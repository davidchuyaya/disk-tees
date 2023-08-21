#!/bin/bash
# 
# Install and run benchbase.
#
cd /home/azureuser
git clone https://github.com/davidchuyaya/disk-tees.git
cd disk-tees
./install.sh
cloud_scripts/db_benchmark/benchbase_install.sh
# TODO: Acquire IP address of client? Then pass to benchbase_run.sh script
CLIENT_IP=10.0.0.4 # Replace
cloud_scripts/db_benchmark/benchbase_run.sh -i $CLIENT_IP
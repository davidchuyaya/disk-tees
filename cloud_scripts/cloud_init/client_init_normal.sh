#!/bin/bash
# 
# Install and run postgres on a single machine.
#
git clone https://github.com/davidchuyaya/disk-tees.git /home/azureuser/disk-tees
cd /home/azureuser/disk-tees
cloud_scripts/db_benchmark/postgres_install.sh
DIR="/home/azureuser/postgres_normal"
mkdir $DIR
cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
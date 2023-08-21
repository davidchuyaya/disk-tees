#!/bin/bash
# 
# Install and run postgres on a single machine.
#
cd /home/azureuser
git clone https://github.com/davidchuyaya/disk-tees.git
cd /home/azureuser/disk-tees
cloud_scripts/db_benchmark/postgres_install.sh
DIR="/home/azureuser/postgres_normal"
mkdir $DIR
cloud_scripts/db_benchmark/postgres_run.sh -d $DIR
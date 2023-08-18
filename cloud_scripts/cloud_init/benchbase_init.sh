#!/bin/bash
# 
# Install and run benchbase.
#
git clone https://github.com/davidchuyaya/disk-tees.git
cd disk-tees
cloud_scripts/db_benchmark/benchbase_install.sh
# TODO: Acquire IP address of client? Then pass to benchbase_run.sh script
cloud_scripts/db_benchmark/benchbase_run.sh
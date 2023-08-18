#!/bin/bash
#
# Running benchbase, assuming postgres has been setup and the IP is known
#
cd ~/benchbase/target/benchbase-postgres
java -jar benchbase.jar -b tpcc -c config/postgres/sample_tpcc_config.xml --create=true --load=true --execute=true
# TODO: Collect results
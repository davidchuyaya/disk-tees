#!/bin/bash
#
# Running benchbase, assuming postgres has been setup and the IP is known
#
print_usage() {
    echo "Usage: $0 -i <client IP address> -u <username>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'i:u:' flag; do
  case ${flag} in
    i) CLIENT_IP=${OPTARG} ;;
    u) USERNAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

NUM_RUNS=3
NUM_WAREHOUSES=100
NUM_TERMINALS=50

cd /home/$USERNAME/benchbase/target/benchbase-postgres
# Modify IP address in config
sed -i "s/localhost/$CLIENT_IP/g" config/postgres/sample_tpcc_config.xml
# Modify warehouses
sed -i "s=<scalefactor>1</scalefactor>=<scalefactor>$NUM_WAREHOUSES</scalefactor>=g" config/postgres/sample_tpcc_config.xml
# Modify terminals
sed -i "s=<terminals>1</terminals>=<terminals>$NUM_TERMINALS</terminals>=g" config/postgres/sample_tpcc_config.xml
# Turn wait times back on
sed -i "s=<!--<preExecutionWait>\(.*\)</preExecutionWait>-->=<preExecutionWait>\1</preExecutionWait>=" config/postgres/sample_tpcc_config.xml
sed -i "s=<!--<postExecutionWait>\(.*\)</postExecutionWait>-->=<postExecutionWait>\1</postExecutionWait>=" config/postgres/sample_tpcc_config.xml
for i in $(seq 1 $NUM_RUNS)
do
    echo "Run $i of $NUM_RUNS"
    java -jar benchbase.jar -b tpcc -c config/postgres/sample_tpcc_config.xml --clear=true --create=true --load=true --execute=true
done
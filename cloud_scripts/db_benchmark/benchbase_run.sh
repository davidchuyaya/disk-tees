#!/bin/bash
#
# Running benchbase, assuming postgres has been setup and the IP is known
#
print_usage() {
    echo "Usage: $0 -i <client IP address> -t <trusted mode>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'i:t:' flag; do
  case ${flag} in
    i) CLIENT_IP=${OPTARG} ;;
    t) TRUSTED_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

NUM_RUNS=3

if [ $TRUSTED_MODE == "local" ]; then
    cd ~
else
    cd /home/azureuser
fi

cd benchbase/target/benchbase-postgres
# Modify IP address in config
sed -i "s/localhost/$CLIENT_IP/g" config/postgres/sample_tpcc_config.xml
for i in $(seq 1 $NUM_RUNS)
do
    echo "Run $i of $NUM_RUNS"
    java -jar benchbase.jar -b tpcc -c config/postgres/sample_tpcc_config.xml --create=true --load=true --execute=true
done
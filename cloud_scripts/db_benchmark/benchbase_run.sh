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

CONFIG=/home/$USERNAME/disk-tees/cloud_scripts/db_benchmark/tpcc_config.xml
# Modify IP address in config
sed -i "s/localhost/$CLIENT_IP/g" $CONFIG
cd /home/$USERNAME/benchbase/target/benchbase-postgres
java -jar benchbase.jar -b tpcc -c $CONFIG --clear=true --create=true --load=true --execute=true
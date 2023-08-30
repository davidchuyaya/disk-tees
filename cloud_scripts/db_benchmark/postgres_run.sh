#!/bin/bash
#
# Create a directory for postgres data and run
#
print_usage() {
    echo "Usage: $0 -d <directory>"
    echo "  -d: directory for postgres"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'd:' flag; do
  case ${flag} in
    d) DIR=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Create subdirectory for postgres. Necessary because postgres wants to modify the permissions(?) and can't do it if that's where tmpfs is directly mounted
SUB_DIR=${DIR}/data
mkdir $SUB_DIR
/usr/local/pgsql/bin/initdb -D $SUB_DIR
# Edit config so postgres listens to public addresses
echo "listen_addresses = '*'" >> $SUB_DIR/postgresql.conf
# Connections don't need to authenticate (only safe for benchmarking)
echo "host all all 0.0.0.0/0 trust" >> $SUB_DIR/pg_hba.conf
/usr/local/pgsql/bin/pg_ctl -D $SUB_DIR -l logfile start
/usr/local/pgsql/bin/createuser -s -i -d -r -l -w admin
/usr/local/pgsql/bin/createdb benchbase
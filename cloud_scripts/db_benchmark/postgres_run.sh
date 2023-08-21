#!/bin/bash
#
# Create a directory for postgres data and run
#
print_usage() {
    echo "Usage: $0 -d <directory>"
    echo "  -d: directory for postgres data"
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

/usr/local/pgsql/bin/initdb -D $DIR
# Edit config so postgres listens to public addresses
echo "listen_addresses = '*'" >> $DIR/postgresql.conf
# Connections don't need to authenticate (only safe for benchmarking)
echo "host all all 0.0.0.0/0 trust" >> $DIR/pg_hba.conf
/usr/local/pgsql/bin/pg_ctl -D $DIR -l logfile start
/usr/local/pgsql/bin/createuser -s -i -d -r -l -w admin
/usr/local/pgsql/bin/createdb benchbase
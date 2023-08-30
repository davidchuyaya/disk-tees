#!/bin/bash
#
#  Create certificate and private key
#
print_usage() {
    echo "Usage: $0 -t <trusted mode> -n <name>"
}

while getopts 't:n:' flag; do
  case ${flag} in
    t) TRUSTED_MODE=${OPTARG} ;;
    n) NAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

if [ $TRUSTED_MODE == "local" ]; then
  IP="127.0.0.1"
else
  IP=$(hostname -I | xargs)
fi

openssl req -x509 -newkey rsa:4096 -keyout ${NAME}_key.pem -out ${NAME}_cert.pem \
  -sha256 -days 365 -nodes -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Microsoft/OU=ConfidentialComputing/CN=$IP"
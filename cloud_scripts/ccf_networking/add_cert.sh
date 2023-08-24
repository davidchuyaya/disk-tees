#!/bin/bash
print_usage() {
    echo "Usage: $0 -i <ID> -n <name>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'i:n:' flag; do
  case ${flag} in
    i) ID=${OPTARG} ;;
    n) NAME=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Base64 encode it to remove newlines in the cert, which breaks JSON: https://stackoverflow.com/a/63108992/4028758
CERT=$(base64 -w 0 ${NAME}_cert.pem)
curl https://127.0.0.1:8000/matchmaker/addcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": '"$ID"', "cert": "'"$CERT"'"}'
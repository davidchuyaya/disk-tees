#!/bin/bash
#
# Send a message to CCF at the specified endpoint and download the results
#
print_usage() {
    echo "Usage: $0 -a <address> -e <endpoint> -j <JSON>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'a:e:j:' flag; do
  case ${flag} in
    a) ADDRESS=${OPTARG} ;;
    e) ENDPOINT=${OPTARG} ;;
    j) JSON=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# -m 60 = Time out after 60 seconds
curl https://${ADDRESS}/matchmaker/${ENDPOINT} -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -m 60 -H "Content-Type: application/json" --data-binary "$JSON" > ${ENDPOINT}.json
#!/bin/bash
print_usage() {
    echo "Usage: $0 -a <API> -f <JSON FILE>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'a:f:' flag; do
  case ${flag} in
    a) API=${OPTARG} ;;
    f) JSON=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

curl https://127.0.0.1:8000/matchmaker/${API} -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary @${JSON} > ${API}.json
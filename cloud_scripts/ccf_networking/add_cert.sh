#!/bin/bash
#
# Attempt to upload a certificate to CCF, retrying until successful
#
print_usage() {
    echo "Usage: $0 -a <address> -i <ID> -n <name> -t <type>"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'a:i:n:t:' flag; do
  case ${flag} in
    a) ADDRESS=${OPTARG} ;;
    i) ID=${OPTARG} ;;
    n) NAME=${OPTARG} ;;
    t) TYPE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Base64 encode it to remove newlines in the cert, which breaks JSON: https://stackoverflow.com/a/63108992/4028758
CERT=$(base64 -w 0 ${NAME}_cert.pem)
OUTPUT=add_cert.json
# Retry until successful, waiting 5 seconds between each retry
while true
do
  # -m 60 = Time out after 60 seconds
  curl https://${ADDRESS}/matchmaker/addcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -m 60 -H "Content-Type: application/json" --data-binary '{"id": '"$ID"', "cert": "'"$CERT"'", "type": "'"$TYPE"'"}' -o $OUTPUT
  if [ $? -eq 0 ]; then
    SUCCESS=$(jq '.success' $OUTPUT)
    if [ $SUCCESS == "true" ]; then
      break
    fi
  fi
  echo "Failed to add cert, retrying in 5s..."
  sleep 5
done

echo "Successfully added cert"
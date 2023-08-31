#!/bin/bash
#
# Attempt to download a certificate from CCF, retrying until successful
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


OUTPUT=get_cert.json
CERT_NAME=${NAME}_cert.pem

if [ -f $CERT_NAME ]; then
  echo "Certificate already exists: $CERT_NAME"
  exit 0
fi

# -m 60 = Time out after 60 seconds
curl https://${ADDRESS}/matchmaker/getcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -m 60 -H "Content-Type: application/json" --data-binary '{"id": '"$ID"', "type": "'"$TYPE"'"}' -o $OUTPUT
if [ $? -eq 0 ]; then
  SUCCESS=$(jq '.success' $OUTPUT)
  if [ $SUCCESS == "true" ]; then
    # Parse output
    CERT64=$(jq -r '.cert' $OUTPUT)
    CERT=$(echo $CERT64 | base64 -d)
    # Need quotes around $CERT to preserve newlines: https://stackoverflow.com/a/22101842/4028758
    echo "$CERT" > $CERT_NAME
    echo "Successfully downloaded cert for id: $ID, name: $NAME"
    # Necessary for all certs under CApath: https://stackoverflow.com/a/32451318/4028758
    c_rehash .
  fi
fi


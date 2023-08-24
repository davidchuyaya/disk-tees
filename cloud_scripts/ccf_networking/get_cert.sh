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

# Retry until a certificate is received
RECEIVED_CERT=false
while true; do
  JSON=$(curl https://127.0.0.1:8000/matchmaker/getcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": '"$ID"'}')
  RECEIVED_CERT=$(jq 'has("cert")' <<< $JSON)
  if [ "$RECEIVED_CERT" = true ]; then
    break
  fi
  # Wait before retrying
  sleep 5
done

# Parse output
CERT64=$(jq -r '.cert' <<< $JSON)
CERT=$(echo $CERT64 | base64 -d)
# Need quotes around $CERT to preserve newlines: https://stackoverflow.com/a/22101842/4028758
echo "$CERT" > ${NAME}_cert.pem
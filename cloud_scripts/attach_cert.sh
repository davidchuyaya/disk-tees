#!/bin/bash
print_usage() {
    echo "Usage: $0 -c <cert file> -i <input bash> -o <output bash> -d <directory>"
    echo "Prepends the certificate to the top of the bash file and adds a line in that file to output the certificate to the directory"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'c:i:o:d:' flag; do
  case ${flag} in
    c) CERT=${OPTARG} ;;
    i) INPUT=${OPTARG} ;;
    o) OUTPUT=${OPTARG} ;;
    d) DIR=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Remove newlines with base64
CERT64=$(base64 -w 0 $CERT)
# Remove the .pem extension for the bash variable name
CERT_PARAM_NAME=$(basename $CERT .pem)
# Create a line to decode the certificate and output it
RECREATE_CERT='echo $'${CERT_PARAM_NAME}' | base64 -d > '${DIR}/${CERT}
# Insert the variable and line
sed "2i ${CERT_PARAM_NAME}=${CERT64} \n ${RECREATE_CERT}" $INPUT > $OUTPUT
chmod +x $OUTPUT
#!/bin/bash
print_usage() {
    echo "Usage: $0 -f <file> -i <input bash> -o <output bash> -d <directory>"
    echo "Prepends the file to the top of the bash script and adds a line in that script to output the file to the specified directory"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'f:i:o:d:' flag; do
  case ${flag} in
    f) FILE=${OPTARG} ;;
    i) INPUT=${OPTARG} ;;
    o) OUTPUT=${OPTARG} ;;
    d) DIR=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Remove newlines with base64
FILE64=$(base64 -w 0 $FILE)
# Remove the extension for the bash variable name
FILE_NAME=$(echo "${FILE%.*}")
# Create a line to decode the file and output it
RECREATE_FILE='echo $'${FILE_NAME}' | base64 -d > '${DIR}/${FILE}
# Insert the variable and line
sed "2i ${FILE_NAME}=${FILE64} \n ${RECREATE_FILE}" $INPUT > $OUTPUT
chmod +x $OUTPUT
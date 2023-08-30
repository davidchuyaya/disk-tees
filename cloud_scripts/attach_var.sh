#!/bin/bash
print_usage() {
    echo "Usage: $0 -i <input bash> -o <output bash> -n <variable name> -v <variable value>"
    echo "Prepends the variable to the top of the bash file"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 'i:o:n:v:' flag; do
  case ${flag} in
    i) INPUT=${OPTARG} ;;
    o) OUTPUT=${OPTARG} ;;
    n) NAME=${OPTARG} ;;
    v) VALUE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Insert the line
sed "2i ${NAME}=${VALUE}" $INPUT > $OUTPUT
chmod +x $OUTPUT
#!/bin/bash
echo "Cleaning up, will take a while. Feel free to exit this script."

VM_NAME="disk_tee"
RESOURCE_GROUP=${VM_NAME}_untrusted

az group delete \
  --name $RESOURCE_GROUP \
  --yes
  
echo "Cleanup complete."
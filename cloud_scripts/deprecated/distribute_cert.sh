#!/bin/bash
# Downloads certificates from Azure VMs once they are all running and distributes them back to the VMs, along with network addresses.
VM_NAME="disk_tee"
RESOURCE_GROUP=${VM_NAME}_untrusted
NUM_VMS=3
CLEANUP=true

# Quit entire script on Ctrl+C.
trap "echo; exit" INT

exitCode=1
while [ $exitCode -eq 1 ]
do
    echo "Checking if VMs have started..."
    az vm list \
    --resource-group $RESOURCE_GROUP \
    --show-details \
    --query "[?powerState=='VM running']" \
    --output json > runningVMs.json

    python3 distribute_cert.py $NUM_VMS
    exitCode=$?
done

if [ $exitCode -eq 0 ]
then
    echo "VMs have launched and certificates were distributed."
fi

# Cleanup
if $CLEANUP;
then
    rm runningVMs.json
    rm network.json
    rm *.pem
fi
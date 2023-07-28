#!/bin/bash
SUBSCRIPTION="d8583813-7f3b-43a6-85ac-bac7e4751e5a" # Azure subscription internal to Microsoft Research. Change this to your own subscription.
LOCATION="swedencentral" # Before changing LOCATION or VM_SIZE, make sure the new location has VMs of that size available.
VM_NAME="disk_tee"
RESOURCE_GROUP=${VM_NAME}_untrusted
VM_SIZE="Standard_D8as_v5"
NUM_VMS=3

# Create a resource group
az group create \
  --subscription $SUBSCRIPTION \
  --name $RESOURCE_GROUP \
  --location $LOCATION

# Create a proximity placement group for fast networking
az ppg create \
  --resource-group $RESOURCE_GROUP \
  --name ${VM_NAME}_ppg \
  --location $LOCATION \
  --intent-vm-sizes $VM_SIZE

# Create VMs
az vm create \
  --resource-group $RESOURCE_GROUP \
  --name $VM_NAME \
  --count $NUM_VMS \
  --admin-username azureuser \
  --generate-ssh-keys \
  --public-ip-sku Standard \
  --nic-delete-option Delete \
  --os-disk-delete-option Delete \
  --custom-data cloud-init.sh \
  --accelerated-networking \
  --ppg ${VM_NAME}_ppg \
  --location $LOCATION \
  --size $VM_SIZE \
  --image Canonical:0001-com-ubuntu-server-focal:20_04-lts:latest 
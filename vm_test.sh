#!/bin/bash
SUBSCRIPTION="d8583813-7f3b-43a6-85ac-bac7e4751e5a"
RG=vm_latency_test_swedencentral_2
ZONE=2

az group create \
  --subscription $SUBSCRIPTION \
  --name $RG \
  --location swedencentral

az ppg create \
  --resource-group $RG \
  --name tee_benchmark_ppg \
  --location swedencentral \
  --zone $ZONE \
  --intent-vm-sizes Standard_D8as_v5

az vm create \
  --resource-group $RG \
  --name tee_benchmark \
  --count 2 \
  --admin-username azureuser \
  --generate-ssh-keys \
  --public-ip-sku Standard \
  --nic-delete-option Delete \
  --os-disk-delete-option Delete \
  --ppg tee_benchmark_ppg \
  --location swedencentral \
  --zone $ZONE \
  --size Standard_D8as_v5 \
  --image Canonical:0001-com-ubuntu-server-focal:20_04-lts:latest 
#!/bin/bash
print_usage() {
  printf "Usage: ./$0 -t <trusted_mode> -p <postgres_mode>\n"
  printf "Options:\n"
  printf "  -t <trusted_mode>    Options: trusted, untrusted\n"
  printf "  -p <postgres_mode>   Options: normal (postgres on a single machine)\n"
  printf "                             tmpfs (postgres on a single machine, writing to tmpfs)\n"
  printf "                             fuse (postgres on a single machine, writing to tmpfs, redirected through fuse)\n"
  printf "                             rollbaccine (postgres + 2f+1 disk TEEs)\n"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

while getopts 't:p:' flag; do
  case ${flag} in
    t) TRUSTED_MODE=${OPTARG} ;;
    p) POSTGRES_MODE=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Validate options and set variables
SUBSCRIPTION="d8583813-7f3b-43a6-85ac-bac7e4751e5a" # Azure subscription internal to Microsoft Research. Change this to your own subscription.
RESOURCE_GROUP=rollbaccine_${TRUSTED_MODE}_${POSTGRES_MODE}
BENCHBASE_INIT_SCRIPT="cloud_scripts/cloud_init/benchbase_init.sh"
REPLICA_INIT_SCRIPT="cloud_scripts/cloud_init/replica_init.sh"
NUM_REPLICAS=3

# Before changing LOCATION, ZONE, or VM_SIZE, make sure the new location has VMs of that size available.
case $TRUSTED_MODE in
  "trusted")
    LOCATION="northeurope"
    ZONE=2
    VM_SIZE="Standard_DC8as_v5"
    IMAGE="Canonical:0001-com-ubuntu-confidential-vm-focal:20_04-lts-cvm:latest"
    TRUSTED_PARAMS="--security-type ConfidentialVM
                    --os-disk-security-encryption-type VMGuestStateOnly
                    --enable-vtpm";;
  "untrusted")
    LOCATION="swedencentral"
    ZONE=2
    VM_SIZE="Standard_D8as_v5"
    IMAGE="Canonical:0001-com-ubuntu-server-focal:20_04-lts:latest";;
  *)
    echo "Invalid trusted mode selected."
    print_usage
    exit 1;;
esac
case $POSTGRES_MODE in
  "normal")
    CLIENT_INIT_SCRIPT="cloud_scripts/cloud_init/client_init_normal.sh";;
  "tmpfs")
    CLIENT_INIT_SCRIPT="cloud_scripts/cloud_init/client_init_tmpfs.sh";;
  "fuse")
    CLIENT_INIT_SCRIPT="cloud_scripts/cloud_init/client_init_fuse.sh";;
  "rollbaccine")
    CLIENT_INIT_SCRIPT="cloud_scripts/cloud_init/client_init_rollbaccine.sh";;
  *)
    echo "Invalid postgres mode selected."
    print_usage
    exit 1;;
esac


# Create a resource group
az group create \
  --subscription $SUBSCRIPTION \
  --name $RESOURCE_GROUP \
  --location $LOCATION

# Create a proximity placement group for fast networking
az ppg create \
  --resource-group $RESOURCE_GROUP \
  --name ${RESOURCE_GROUP}_ppg \
  --location $LOCATION \
  --intent-vm-sizes $VM_SIZE

# Arguments: $1 = "client", "replica", or "benchbase"
create_vms() {
  case $1 in
    "client")
      INIT_SCRIPT=$CLIENT_INIT_SCRIPT;;
    "replica")
      REPLICA_COUNT="--count $NUM_REPLICAS"
      INIT_SCRIPT=$REPLICA_INIT_SCRIPT;;
    "benchbase")
      INIT_SCRIPT=$BENCHBASE_INIT_SCRIPT;;
    *) 
      echo "Invalid VM type in create_vms() of launch.sh."
      exit 1;;
  esac

  az vm create \
    --resource-group $RESOURCE_GROUP \
    --name ${RESOURCE_GROUP}_$1 \
    --admin-username azureuser \
    --generate-ssh-keys \
    --public-ip-sku Standard \
    --nic-delete-option Delete \
    --os-disk-delete-option Delete \
    --custom-data $INIT_SCRIPT \
    --accelerated-networking \
    --ppg ${RESOURCE_GROUP}_ppg \
    --location $LOCATION \
    --zone $ZONE \
    --size $VM_SIZE \
    --image $IMAGE $REPLICA_COUNT $TRUSTED_PARAMS
}

# Create VMs
create_vms "client"
create_vms "benchbase"
if [ $POSTGRES_MODE == "rollbaccine" ]
then
  create_vms "replica"
fi
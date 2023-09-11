#!/bin/bash
print_usage() {
  printf "Usage: $0 -t <trusted mode> -p <postgres mode> [-w <wait secs before running> -m <tmpfs memory size (GB)>]\n"
  printf "Options:\n"
  printf "  -t <trusted mode>    Options: trusted, untrusted, local\n"
  printf "  -p <postgres mode>   Options: normal (postgres on a single machine)\n"
  printf "                             tmpfs (postgres on a single machine, writing to tmpfs)\n"
  printf "                             fuse (postgres on a single machine, writing to tmpfs, redirected through fuse)\n"
  printf "                             rollbaccine (postgres + 2f+1 disk TEEs)\n"
  printf "  -w <wait secs>       Expected seconds between starting the client and it winning leader election. Only relevant in rollbaccine mode\n"
  printf "  -m <tmpfs memory size (GB)>   How much memory to provide tmpfs\n"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

WAIT_SECS=0
TMPFS_MEMORY=1
while getopts 't:p:w:m:' flag; do
  case ${flag} in
    t) TRUSTED_MODE=${OPTARG} ;;
    p) POSTGRES_MODE=${OPTARG} ;;
    w) WAIT_SECS=${OPTARG} ;;
    m) TMPFS_MEMORY=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

# Set variables
SUBSCRIPTION="d8583813-7f3b-43a6-85ac-bac7e4751e5a" # Azure subscription internal to Microsoft Research. Change this to your own subscription.
RESOURCE_GROUP=rollbaccine_${TRUSTED_MODE}_${POSTGRES_MODE}
NUM_REPLICAS=2
NUM_CCF_NODES=3
CCF_PORT=10200

USERNAME=$(whoami)
PROJECT_DIR=/home/$USERNAME/disk-tees
CLIENT_INIT_SCRIPT_NAME=client_init_${POSTGRES_MODE}.sh
CLIENT_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/${CLIENT_INIT_SCRIPT_NAME}
BENCHBASE_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/benchbase_init.sh
REPLICA_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/replica_init.sh


############################################################################### Run locally
if [ $TRUSTED_MODE == "local" ]
then
  BUILD_DIR=$PROJECT_DIR/build
  mkdir -p $BUILD_DIR
  cd $BUILD_DIR

  if [ $POSTGRES_MODE != "rollbaccine" ]
  then
    # Only launch client
    CLIENT_DIR=$BUILD_DIR/client0
    mkdir -p $CLIENT_DIR
    NEW_INIT_SCRIPT=$CLIENT_DIR/client_init.sh
    # Attach variables
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $CLIENT_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v 0
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TMPFS_MEMORY -v $TMPFS_MEMORY
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n USERNAME -v $USERNAME
    # Run client
    $NEW_INIT_SCRIPT
    exit 0
  fi
  
  # 1. Create nodes for CCF (unnecessary)
  # 2. Launch CCF in the background
  /opt/ccf_virtual/bin/sandbox.sh --verbose \
    --package /opt/ccf_virtual/lib/libjs_generic \
    --js-app-bundle $PROJECT_DIR/ccf > $BUILD_DIR/ccf.log 2>&1 &
  
  # Create CCF JSON. Note that IP include port, since it'll be used by CURL that way.
  CCF_JSON=$BUILD_DIR/ccf.json
  echo '[{"ip": "127.0.0.1:8000", "id": 0}]' > $CCF_JSON

  # Wait for CCF to launch by checking if certificates have been created
  CERT_DIR=$BUILD_DIR/workspace/sandbox_common
  SERVICE_CERT=$CERT_DIR/service_cert.pem
  USER_CERT=$CERT_DIR/user0_cert.pem
  USER_KEY=$CERT_DIR/user0_privk.pem
  while [ ! -f $SERVICE_CERT ] || [ ! -f $USER_CERT ] || [ ! -f $USER_KEY ]; do
    echo "CCF not yet started, retrying in 5s..."
    sleep 5
  done

  # 3. Launch replicas
  for ((i = 0; i < $NUM_REPLICAS; i++));
  do
    REPLICA_DIR=$BUILD_DIR/replica$i
    mkdir -p $REPLICA_DIR
    NEW_INIT_SCRIPT=$REPLICA_DIR/replica_init.sh
    # Attach CCF certificates and CCF JSON
    $PROJECT_DIR/cloud_scripts/attach_file.sh -f $SERVICE_CERT -i $REPLICA_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_KEY -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $PROJECT_DIR/cloud_scripts/attach_file.sh -f $CCF_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    # Attach variables
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v $i
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TMPFS_MEMORY -v $TMPFS_MEMORY
    $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n USERNAME -v $USERNAME
    # Run replica in background
    $NEW_INIT_SCRIPT > $REPLICA_DIR/log.txt 2>&1 &
    sleep 1 # Prevent 2 replicas from trying to overwrite the same executable through cmake
  done

  # Create replica JSON
  REPLICA_JSON=$BUILD_DIR/replicas.json
  echo '[' > $REPLICA_JSON
  for ((i = 0; i < $NUM_REPLICAS; i++));
  do
    if [ $i != 0 ]
    then
      echo ',' >> $REPLICA_JSON
    fi
    echo '{"ip": "127.0.0.1", "id": '$i'}' >> $REPLICA_JSON
  done
  echo ']' >> $REPLICA_JSON

  # 4. Launch client
  CLIENT_DIR=$BUILD_DIR/client0
  mkdir -p $CLIENT_DIR
  NEW_INIT_SCRIPT=$CLIENT_DIR/client_init.sh
  # Attach CCF certificates and CCF JSON
  $PROJECT_DIR/cloud_scripts/attach_file.sh -f $SERVICE_CERT -i $CLIENT_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_KEY -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $PROJECT_DIR/cloud_scripts/attach_file.sh -f $CCF_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $PROJECT_DIR/cloud_scripts/attach_file.sh -f $REPLICA_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  # Attach variables
  $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
  $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v 0
  $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n WAIT_SECS -v $WAIT_SECS
  $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TMPFS_MEMORY -v $TMPFS_MEMORY
  $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n USERNAME -v $USERNAME
  # Run client
  $NEW_INIT_SCRIPT

  exit 0
fi


############################################################################### Run on azure
# Before changing LOCATION, ZONE, or VM_SIZE, make sure the new location has VMs of that size available.
case $TRUSTED_MODE in
  "trusted")
    LOCATION="northeurope"
    ZONE=3
    VM_SIZE="Standard_DC16as_v5"
    CCF_VM_SIZE="Standard_DC2as_v5";; # Use as few cores as possible for CCF since it's not on the critical path
  "untrusted")
    LOCATION="swedencentral"
    ZONE=2
    VM_SIZE="Standard_D16as_v5";;
  *)
    echo "Invalid trusted mode selected."
    print_usage
    exit 1;;
esac
BENCHBASE_VM_SIZE="Standard_D32_v5" # 32 cores for benchbase, doesn't need to be confidential

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
  --zone $ZONE \
  --intent-vm-sizes $VM_SIZE $CCF_VM_SIZE $BENCHBASE_VM_SIZE

# Parameters: $1 = name of node (ccf, benchbase, or client_and_replicas)
create_vms() {
  UNTRUSTED_IMAGE="Canonical:0001-com-ubuntu-server-focal:20_04-lts:latest"
  TRUSTED_IMAGE="Canonical:0001-com-ubuntu-confidential-vm-focal:20_04-lts-cvm:latest"
  TRUSTED_PARAMS="--security-type ConfidentialVM
                    --os-disk-security-encryption-type VMGuestStateOnly
                    --enable-vtpm"
  PARAMS="" # Reset params
  case $1 in
    "ccf")
      IMAGE=$TRUSTED_IMAGE
      SIZE=$CCF_VM_SIZE
      PARAMS=$TRUSTED_PARAMS" --count "$NUM_CCF_NODES;;
    "benchbase")
      IMAGE=$UNTRUSTED_IMAGE
      SIZE=$BENCHBASE_VM_SIZE;;
    *)
      if [[ $TRUSTED_MODE == "trusted" ]]
      then
        IMAGE=$TRUSTED_IMAGE
        if [[ $POSTGRES_MODE == "rollbaccine" ]]
        then # VMs for replicas + client
          NUM_REPLICAS_AND_CLIENT=$(($NUM_REPLICAS + 1))
          PARAMS=$TRUSTED_PARAMS" --data-disk-sizes-gb 32 --count "$NUM_REPLICAS_AND_CLIENT
        else # VMs for just client
          PARAMS=$TRUSTED_PARAMS" --data-disk-sizes-gb 32"
        fi
      else
        IMAGE=$UNTRUSTED_IMAGE
      fi
      SIZE=$VM_SIZE;;
  esac

  # Note: Turn accelerated networking off because it's available for VMs but not CVMs
  az vm create \
    --resource-group $RESOURCE_GROUP \
    --name $1 \
    --admin-username $USERNAME \
    --generate-ssh-keys \
    --public-ip-sku Standard \
    --nic-delete-option Delete \
    --os-disk-delete-option Delete \
    --data-disk-delete-option Delete \
    --accelerated-networking false \
    --ppg ${RESOURCE_GROUP}_ppg \
    --location $LOCATION \
    --zone $ZONE \
    --size $SIZE \
    --image $IMAGE $PARAMS
}

create_vms client_and_replicas
create_vms benchbase

# Allow CCF nodes to listen to WSL on $CCF_PORT
if [[ $POSTGRES_MODE == "rollbaccine" ]]
then
  create_vms ccf
  az network nsg rule create \
    --resource-group $RESOURCE_GROUP \
    --nsg-name ${POSTGRES_MODE}NSG \
    --name allow_ccf \
    --priority 1010 \
    --destination-port-ranges $CCF_PORT
fi
#!/bin/bash
print_usage() {
  printf "Usage: ./$0 -t <trusted mode> -p <postgres mode>\n"
  printf "Options:\n"
  printf "  -t <trusted mode>    Options: trusted, untrusted, local\n"
  printf "  -p <postgres mode>   Options: normal (postgres on a single machine)\n"
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
NUM_REPLICAS=3

# Before changing LOCATION, ZONE, or VM_SIZE, make sure the new location has VMs of that size available.
case $TRUSTED_MODE in
  "trusted")
    PROJECT_DIR=/home/azureuser/disk-tees
    LOCATION="northeurope"
    ZONE=2
    VM_SIZE="Standard_DC8as_v5"
    IMAGE="Canonical:0001-com-ubuntu-confidential-vm-focal:20_04-lts-cvm:latest"
    TRUSTED_PARAMS="--security-type ConfidentialVM
                    --os-disk-security-encryption-type VMGuestStateOnly
                    --enable-vtpm";;
  "untrusted")
    PROJECT_DIR=/home/azureuser/disk-tees
    LOCATION="swedencentral"
    ZONE=2
    VM_SIZE="Standard_D8as_v5"
    IMAGE="Canonical:0001-com-ubuntu-server-focal:20_04-lts:latest";;
  "local")
    PROJECT_DIR=~/disk-tees;;
  *)
    echo "Invalid trusted mode selected."
    print_usage
    exit 1;;
esac
CLIENT_INIT_SCRIPT_NAME=client_init_${POSTGRES_MODE}.sh
# Note: All these scripts start with ~/disk-tees because they are only used by the launcher, and will be copied via cloud-init to the VMs.
CLIENT_INIT_SCRIPT=~/disk-tees/cloud_scripts/cloud_init/${CLIENT_INIT_SCRIPT_NAME}
BENCHBASE_INIT_SCRIPT=~/disk-tees/cloud_scripts/cloud_init/benchbase_init.sh
REPLICA_INIT_SCRIPT=~/disk-tees/cloud_scripts/cloud_init/replica_init.sh


############################################################################### Run locally
if [ $TRUSTED_MODE == "local" ]
then
  BUILD_DIR=$PROJECT_DIR/build
  mkdir $BUILD_DIR
  cd $BUILD_DIR

  if [ $POSTGRES_MODE != "rollbaccine" ]
  then
    
  fi
  
  # 1. Create nodes for CCF (unnecessary)
  # 2. Launch CCF in the background
  /opt/ccf_virtual/bin/sandbox.sh --verbose \
    --package /opt/ccf_virtual/lib/libjs_generic \
    --js-app-bundle $PROJECT_DIR/ccf &
  
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
    mkdir $REPLICA_DIR
    NEW_INIT_SCRIPT=$REPLICA_DIR/replica_init.sh
    # Attach CCF certificates and CCF JSON
    $HOME_DIR/cloud_scripts/attach_file.sh -f $SERVICE_CERT -i $REPLICA_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $HOME_DIR/cloud_scripts/attach_file.sh -f $USER_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $HOME_DIR/cloud_scripts/attach_file.sh -f $USER_KEY -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    $HOME_DIR/cloud_scripts/attach_file.sh -f $CCF_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $REPLICA_DIR
    # Attach variables
    $HOME_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
    $HOME_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v $i
    # Run replica in background
    $NEW_INIT_SCRIPT &
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
  CLIENT_DIR=$BUILD_DIR/client
  mkdir $CLIENT_DIR
  NEW_INIT_SCRIPT=$CLIENT_DIR/client_init.sh
  # Attach CCF certificates and CCF JSON
  $HOME_DIR/cloud_scripts/attach_file.sh -f $SERVICE_CERT -i $CLIENT_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $HOME_DIR/cloud_scripts/attach_file.sh -f $USER_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $HOME_DIR/cloud_scripts/attach_file.sh -f $USER_KEY -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $HOME_DIR/cloud_scripts/attach_file.sh -f $CCF_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  $HOME_DIR/cloud_scripts/attach_file.sh -f $REPLICA_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
  # Attach variables
  $HOME_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
  $HOME_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v 0
  # Run client
  $BUILD_DIR/client/client_init.sh

  exit 0
fi


############################################################################### Run on azure
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
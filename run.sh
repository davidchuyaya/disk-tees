#!/bin/bash
print_usage() {
  printf "Usage: $0 -t <trusted mode> -p <postgres mode> -n <node type> [-w <wait secs before running> -m <tmpfs memory size (GB)>]\n"
  printf "Options:\n"
  printf "  -t <trusted mode>    Options: trusted, untrusted\n"
  printf "  -p <postgres mode>   Options: normal (postgres on a single machine)\n"
  printf "                             tmpfs (postgres on a single machine, writing to tmpfs)\n"
  printf "                             fuse (postgres on a single machine, writing to tmpfs, redirected through fuse)\n"
  printf "                             rollbaccine (postgres + 2f+1 disk TEEs)\n"
  printf "  -n <node type>       Options: client, benchbase, ccf, replicas\n"
  printf "  -w <wait secs>       Expected seconds between starting the client and it winning leader election. Only relevant in rollbaccine mode\n"
  printf "  -m <tmpfs memory size (GB)>   How much memory to provide tmpfs\n"
}

if (( $# == 0 )); then
    print_usage
    exit 1
fi

WAIT_SECS=0
TMPFS_MEMORY=1
while getopts 't:p:n:w:m:' flag; do
  case ${flag} in
    t) TRUSTED_MODE=${OPTARG} ;;
    p) POSTGRES_MODE=${OPTARG} ;;
    n) NODE_TYPE=${OPTARG} ;;
    w) WAIT_SECS=${OPTARG} ;;
    m) TMPFS_MEMORY=${OPTARG} ;;
    *) print_usage
       exit 1;;
  esac
done

RESOURCE_GROUP=rollbaccine_${TRUSTED_MODE}_${POSTGRES_MODE}
CCF_PORT=10200

echo "Fetching VM IP addresses, if necessary, assuming launch.sh has been run..."
VMS_JSON=build/vms.json
if [ ! -f $VMS_JSON ]; then
    az vm list \
        --resource-group $RESOURCE_GROUP \
        --show-details > $VMS_JSON
fi

# Assign IP addresses to variables
BENCHBASE_PUBLIC_IP=$(jq -r '.[] | select(.name=="benchbase") | .publicIps' $VMS_JSON)
CLIENT_PUBLIC_IP=$(jq -r '.[] | select(.name=="client_and_replicas") | .publicIps' $VMS_JSON)
CLIENT_PRIVATE_IP=$(jq -r '.[] | select(.name=="client_and_replicas") | .privateIps' $VMS_JSON)
if [ -z "$CLIENT_PUBLIC_IP" ]
then
    # Must've launched client with replicas. Gather all IPs
    CLIENT_PUBLIC_IP=$(jq -r '.[] | select(.name=="client_and_replicas0") | .publicIps' $VMS_JSON)
    CLIENT_PRIVATE_IP=$(jq -r '.[] | select(.name=="client_and_replicas0") | .privateIps' $VMS_JSON)
    readarray -t REPLICA_PUBLIC_IPS < <(jq -r '.[] | select(.name | test("client_and_replicas[1-9]")) | .publicIps' $VMS_JSON)
    readarray -t REPLICA_PRIVATE_IPS < <(jq -r '.[] | select(.name | test("client_and_replicas[1-9]")) | .privateIps' $VMS_JSON)
fi
readarray -t CCF_PUBLIC_IPS < <(jq -r '.[] | select(.name | test("ccf[0-9]")) | .publicIps' $VMS_JSON)
readarray -t CCF_PRIVATE_IPS < <(jq -r '.[] | select(.name | test("ccf[0-9]")) | .privateIps' $VMS_JSON)

echo "Creating JSONs for CCF and replicas, if necessary."
CCF_JSON=build/ccf.json
REPLICAS_JSON=build/replicas.json
if [ ! -f $CCF_JSON ]
then
    echo '[' > $CCF_JSON
    for i in "${!CCF_PRIVATE_IPS[@]}"
    do
        if (( $i > 0 ))
        then
            echo ',' >> $CCF_JSON
        fi
        echo '{"ip": "'${CCF_PRIVATE_IPS[$i]}:$CCF_PORT'", "id": '$i'}' >> $CCF_JSON
    done
    echo ']' >> $CCF_JSON
fi
if [ ! -f $REPLICAS_JSON ]
then
    echo '[' > $REPLICAS_JSON
    for i in "${!REPLICA_PRIVATE_IPS[@]}"
    do
        if (( $i > 0 ))
        then
            echo ',' >> $REPLICAS_JSON
        fi
        echo '{"ip": "'${REPLICA_PRIVATE_IPS[$i]}'", "id": '$i'}' >> $REPLICAS_JSON
    done
    echo ']' >> $REPLICAS_JSON
fi

echo "Running $NODE_TYPE..."
# Note: Most of the code below matches "local" mode in launch.sh
USERNAME=$(whoami)
PROJECT_DIR=/home/$USERNAME/disk-tees
CLIENT_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/client_init_${POSTGRES_MODE}.sh
BENCHBASE_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/benchbase_init.sh
REPLICA_INIT_SCRIPT=$PROJECT_DIR/cloud_scripts/cloud_init/replica_init.sh

BUILD_DIR=$PROJECT_DIR/build
CERT_DIR=$BUILD_DIR/workspace/sandbox_common
SERVICE_CERT=$CERT_DIR/service_cert.pem
USER_CERT=$CERT_DIR/user0_cert.pem
USER_KEY=$CERT_DIR/user0_privk.pem

case $NODE_TYPE in
    "client")
        CLIENT_DIR=$BUILD_DIR/client0
        mkdir -p $CLIENT_DIR
        NEW_INIT_SCRIPT=$CLIENT_DIR/client_init.sh
        cp $CLIENT_INIT_SCRIPT $NEW_INIT_SCRIPT
        if [ $POSTGRES_MODE == "rollbaccine" ]
        then
            # Attach CCF certificates and CCF JSON
            $PROJECT_DIR/cloud_scripts/attach_file.sh -f $SERVICE_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
            $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_CERT -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
            $PROJECT_DIR/cloud_scripts/attach_file.sh -f $USER_KEY -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
            $PROJECT_DIR/cloud_scripts/attach_file.sh -f $CCF_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
            $PROJECT_DIR/cloud_scripts/attach_file.sh -f $REPLICA_JSON -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -d $CLIENT_DIR
        fi
        # Attach variables
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TRUSTED_MODE -v $TRUSTED_MODE
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n ID -v 0
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n WAIT_SECS -v $WAIT_SECS
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n TMPFS_MEMORY -v $TMPFS_MEMORY
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n USERNAME -v $USERNAME
        ssh -o StrictHostKeyChecking=no $USERNAME@$CLIENT_PUBLIC_IP "bash -s" -- < $NEW_INIT_SCRIPT
        ;;
    "benchbase")
        BENCHBASE_DIR=$BUILD_DIR/benchbase
        mkdir -p $BENCHBASE_DIR
        NEW_INIT_SCRIPT=$BENCHBASE_DIR/benchbase_init.sh
        cp $BENCHBASE_INIT_SCRIPT $NEW_INIT_SCRIPT
        # Attach variables
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n CLIENT_IP -v $CLIENT_PRIVATE_IP
        $PROJECT_DIR/cloud_scripts/attach_var.sh -i $NEW_INIT_SCRIPT -o $NEW_INIT_SCRIPT -n USERNAME -v $USERNAME
        ssh -o StrictHostKeyChecking=no $BENCHBASE_PUBLIC_IP "bash -s" -- < $NEW_INIT_SCRIPT
        # Download results
        echo "Downloading results to $BENCHBASE_DIR/results..."
        scp -r $USERNAME@$BENCHBASE_PUBLIC_IP:/home/$USERNAME/benchbase/target/benchbase-postgres/results $BENCHBASE_DIR
        ;;
    "ccf")
        # Create ansible inventory file, nodes argument for sandbox.sh
        export ANSIBLE_HOST_KEY_CHECKING=False
        CCF_ANSIBLE_INVENTORY=build/ccf_ansible_inventory
        CCF_ADDRS=""
        echo '[all]' > $CCF_ANSIBLE_INVENTORY
        for i in "${!CCF_PUBLIC_IPS[@]}"
        do
            CCF_PUBLIC_IP=${CCF_PUBLIC_IPS[$i]}
            CCF_PRIVATE_IP=${CCF_PRIVATE_IPS[$i]}
            echo $CCF_PUBLIC_IP >> $CCF_ANSIBLE_INVENTORY
            CCF_ADDRS+="--node ssh://"$CCF_PRIVATE_IP:$CCF_PORT,$CCF_PUBLIC_IP:$CCF_PORT" "
        done
        # Install CCF dependencies
        ansible-playbook $PROJECT_DIR/cloud_scripts/ccf-deps.yml -i $CCF_ANSIBLE_INVENTORY
        # Run sandbox.sh
        echo "CCF_ADDRS: $CCF_ADDRS"
        cd $BUILD_DIR
        /opt/ccf_virtual/bin/sandbox.sh --verbose \
            --package /opt/ccf_virtual/lib/libjs_generic \
            --js-app-bundle ~/disk-tees/ccf $CCF_ADDRS # List of (local://|ssh://)hostname:port[,pub_hostnames:pub_port]
        ;;
    "replicas")  
        for i in "${!REPLICA_PUBLIC_IPS[@]}"
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
            # TODO: Figure out how to launch multiple without waiting for previous to finish
            ssh -o StrictHostKeyChecking=no $USERNAME@${REPLICA_PUBLIC_IPS[$i]} "bash -s" -- < $NEW_INIT_SCRIPT
        done
        echo "Finished starting replicas"
        ;;
    *) 
        echo "Invalid node type in run.sh."
        print_usage
        exit 1;;
esac
#!/bin/bash

# Install CCF
# Instructions from https://microsoft.github.io/CCF/main/build_apps/install_bin.html
export CCF_VERSION=$(curl -ILs -o /dev/null -w %{url_effective} https://github.com/microsoft/CCF/releases/latest | sed 's/^.*ccf-//')
wget https://github.com/microsoft/CCF/releases/download/ccf-${CCF_VERSION}/ccf_virtual_${CCF_VERSION}_amd64.deb
sudo apt -y install ./ccf_virtual_${CCF_VERSION}_amd64.deb

# Install ansible for setting up CCF on VMs. See https://github.com/microsoft/CCF/blob/main/getting_started/setup_vm/run.sh
sudo apt-get -y update
sudo apt install software-properties-common -y
sudo add-apt-repository -y --update ppa:ansible/ansible
sudo apt install ansible -y

cloud_scripts/benchbase_install.sh -t local
cloud_scripts/postgres_install.sh -t local
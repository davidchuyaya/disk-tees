#!/bin/bash

# Install CCF
# Instructions from https://microsoft.github.io/CCF/main/build_apps/install_bin.html
export CCF_VERSION=$(curl -ILs -o /dev/null -w %{url_effective} https://github.com/microsoft/CCF/releases/latest | sed 's/^.*ccf-//')
wget https://github.com/microsoft/CCF/releases/download/ccf-${CCF_VERSION}/ccf_virtual_${CCF_VERSION}_amd64.deb
sudo apt -y install ./ccf_virtual_${CCF_VERSION}_amd64.deb

cloud_scripts/benchbase_install.sh -t local
cloud_scripts/postgres_install.sh -t local
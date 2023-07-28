#!/bin/bash
sudo apt update
sudo apt -y install cmake build-essential libssl-dev

# Install Azure CLI.
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
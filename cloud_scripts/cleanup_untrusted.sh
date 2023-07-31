#!/bin/bash
echo "Cleaning up, will take a while. Feel free to exit this script."
# Ensure the name here matches the resource group name in launch_untrusted.sh
az group delete \
  --name disk_tee_untrusted \
  --yes
echo "Cleanup complete."
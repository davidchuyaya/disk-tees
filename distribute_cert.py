import sys
import json
import subprocess

# Make sure these file names match what's in distribute_cert.sh
VM_JSON = "runningVMs.json"
NETWORK_JSON = "network.json"
STILL_LAUNCHING_EXIT_CODE = 1
ERROR_EXIT_CODE = 2

def main():
    args = sys.argv[1:]
    if len(args) < 1:
        print('Usage: python3 distribute_cert.py <numVMs>')
        sys.exit(ERROR_EXIT_CODE)
    
    with open(VM_JSON) as file:
        try:
            vms = json.load(file)
        except json.decoder.JSONDecodeError:
            print(f'Error parsing {VM_JSON}; did you specify the wrong resource group or forget to launch the VMs?')
            sys.exit(ERROR_EXIT_CODE)
        num_vms = int(args[0])

        if len(vms) < num_vms:
            print(f'VMs still launching: {len(vms)}/{num_vms}')
            sys.exit(STILL_LAUNCHING_EXIT_CODE)
        if len(vms) > num_vms:
            print(f'Too many VMs listed, something is wrong: {len(vms)}/{num_vms}')
            sys.exit(ERROR_EXIT_CODE)
        
        create_network_json(vms)
        download_certificates(vms)
        distribute_certificates_and_network_json(vms)

"""
Create a JSON file with "name" and "ip" fields for each VM.
"name": The name of the VM AND the name of the certificate file .pem
"ip": The private IP address of the VM
"""
def create_network_json(vms):
    network_json = []
    for vm in vms:
        network_json.append({
            "name": vm['name'],
            "ip": vm['privateIps']
        })
    with open(NETWORK_JSON, 'w') as file:
        json.dump(network_json, file)

def download_certificates(vms):
    for vm in vms:
        name = vm['name']
        publicIp = vm['publicIps']
        # Download each server's certificate and rename it to their name
        subprocess.run(['scp', f'azureuser@{publicIp}:~/server_cert.pem', f'{name}.pem',
                        '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null']) # Don't check host key

def distribute_certificates_and_network_json(vms):
    for vm in vms:
        name = vm['name']
        publicIp = vm['publicIps']
        # Send the network JSON
        subprocess.run(['scp', NETWORK_JSON, f'azureuser@{publicIp}:~/{NETWORK_JSON}'])
        # Send to each server the certificate of all other servers
        for other_vm in vms:
            other_name = other_vm['name']
            if name != other_name:
                subprocess.run(['scp', f'{other_name}.pem', f'azureuser@{publicIp}:~/{other_name}.pem',
                        '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null']) # Don't check host key
    
if __name__ == "__main__":
    main()
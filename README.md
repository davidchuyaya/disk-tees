# disk-tees
2f+1 confidential servers to support a rollback-detecting disk abstraction.

## Installation 
Execute the following commands to run the scripts and executables locally:
```bash
./install.sh
cmake .
make
```

If you want to test locally, you'd need to create the necessary keys and certificates. Execute:
```bash
./create_cert.sh
```


## Creating certificates in Azure
### Untrusted
Execute the following on your local computer:
```bash
cloud_scripts/launch_untrusted.sh
cloud_scripts/distribute_cert.sh #TODO Let servers send certificates to each other instead. Close SSH port so we can't steal server secrets.
```

Note that the script will not work if you are not under the "Azure Research Subs" subscription in Azure. Be sure to modify the script to use your own subscription. More details and configuration parameters can be found at the top of the script.
The VMs in the cloud will run `cloud-init.sh` as part of their startup process and create `server_cert.pem` and `server_key.pem` files under `/home/azureuser/`. The certificates will then be shared between all servers and clients. The key should not leave the server.

## Shutting down Azure
### Untrusted
Execute the following on your local computer:
```bash
cloud_scripts/cleanup_untrusted.sh
```

## Running the replicas
To run `replica/disk_tees`, execute the following;
```bash
replica/disk_tees -i <id>
```
Note that `<id>` must match some ID specified in `network.json`.


## Running the client
To run `client/tee_fuse`, execute the following:
```bash
client/tee_fuse <mountpoint>
```
`<mountpoint>` is the directory where the filesystem will be mounted.
To see all the options, execute `client/tee_fuse -h`. For example, `-f` is used to run the filesystem in the foreground, which can be helpful for debugging and analyzing.

When you're done running the system and want to unmount, execute the following:
```bash
sudo umount <mountpoint>
```


## Libraries
This project makes use of the following C++ libraries:

- [OpenSSL](https://wiki.openssl.org/index.php/Main_Page) - Used for TLS encrypted connections
- [JSON for modern C++](https://github.com/nlohmann/json#examples) - Used for reading JSON config files
- [zpp::bits](https://github.com/eyalz800/zpp_bits) - Used for fast serializing/deserializing of CPP structs. Requires C++20
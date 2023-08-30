# disk-tees
2f+1 confidential servers to support a rollback-detecting disk abstraction.

## Installation 
To run locally, this repo must be cloned to the user's home directory `~` and cannot be renamed, since the absolute location of many scripts is critical to correctness.

Execute the following commands to install the necessary packages to compile this repo:
```bash
./install.sh
cmake .
make
```

(Test local) Execute the following commands to run the benchmarks locally:
```bash
./local_install.sh
```

(Test remote) In order to launch any VMs, you must install the Azure CLI locally as well:
```bash
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
```

## Running locally

If you want to test locally, you'd need to create the necessary keys and certificates. Execute:
```bash
./create_testing_cert.sh
```

TODO: Figure of how writes enter through the mountpoint, are sent to the replicas, and then are written to the redirect point and the replica's directory.

To start the replicas, execute the following;
```bash
replica/disk_tees -i <id> -d <directory>
```
Note that `<id>` must match some ID specified in `network.json`, which can be edited based on the ports you'd like to expose. Documentation for `network.json` can be found in `shared/network_config.hpp`.
`<directory>` is the directory where the replica will store its data. Note that this directory must first be manually created.

To start the client, execute the following:
```bash
client/tee_fuse -f -s -i <id> -r <redirect point> <mountpoint>
```
- `-f` states that the client should run in the foreground, so we can see any error logs.
- `-s` turns on single-threaded mode, which is required for sequentially sequencing writes.
- `-i` is the id, as before.
- `-r` is where the client will store the data.
- `<mountpoint>` is the directory where the filesystem will be mounted.
To see all the options provided by FUSE, execute `client/tee_fuse -h`.

When you're done running the system, or if the system terminates unexpectedly, you may need to unmount by executing the following:
```bash
sudo umount <mountpoint>
```

## Running in Azure

Execute the following:
```bash
./launch.sh -t <trusted_mode> -p <postgres_mode>
```
Documentation for each mode can be found by executing `./launch.sh`. Note that the script will not work if you are not under the "Azure Research Subs" subscription in Azure. Be sure to modify the script to use your own subscription.

TODO: Launch CCF, load replica IP addresses in, then launch client with that config. Might want to break up launch.sh into multiple scripts so it's easier to debug.

### Shutting down Azure
TODO


TODO: The following Azure sections are deprecated but still useful for me, will delete after I'm done writing scripts
## Creating certificates in Azure
### Untrusted
Execute the following on your local computer:
```bash
cloud_scripts/launch_untrusted.sh
cloud_scripts/distribute_cert.sh
```

Note that the script will not work if you are not under the "Azure Research Subs" subscription in Azure. Be sure to modify the script to use your own subscription. More details and configuration parameters can be found at the top of the script.
The VMs in the cloud will run `cloud-init.sh` as part of their startup process and create `server_cert.pem` and `server_key.pem` files under `/home/azureuser/`. The certificates will then be shared between all servers and clients. The key should not leave the server.

## Shutting down Azure
### Untrusted
Execute the following on your local computer:
```bash
cloud_scripts/cleanup_untrusted.sh
```


## Libraries
This project makes use of the following C++ libraries:

- [OpenSSL](https://wiki.openssl.org/index.php/Main_Page) - Used for TLS encrypted connections
- [JSON for modern C++](https://github.com/nlohmann/json#examples) - Used for reading JSON config files
- [zpp::bits](https://github.com/eyalz800/zpp_bits) - Used for fast serializing/deserializing of CPP structs. Requires C++20
- [libfuse](https://github.com/libfuse/libfuse) - For intercepting writes to the file system
- [Fusepp](https://github.com/jachappell/Fusepp) - For wrapping FUSE functions in C++
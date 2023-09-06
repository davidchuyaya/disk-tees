# disk-tees
2f+1 confidential servers to support a rollback-detecting disk abstraction.

## Installation 
To run locally, this repo must be cloned to the user's home directory `~` and cannot be renamed, since the absolute location of many scripts is critical to correctness.

Execute the following commands to install the necessary packages to compile this repo:
```bash
./install.sh
cmake .
make
./local_install.sh
```

(Test remote) In order to launch any VMs, you must install the Azure CLI locally as well:
```bash
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
```

## Running

Execute the following:
```bash
./launch.sh -t <trusted_mode> -p <postgres_mode> -w <wait time> -m <tmpfs memory>
```
Documentation for each mode can be found by executing `./launch.sh`. Note that the script will not work if you are not under the "Azure Research Subs" subscription in Azure. Be sure to modify the script to use your own subscription.

To run locally, use `local` as the trusted mode. Files will be created under the `build` directory. Here are some ways I used `launch.sh` for local testing. Pick one to run:
```bash
./launch.sh -t local -p normal
./launch.sh -t local -p fuse -m 2
./launch.sh -t local -p tmpfs -m 2
./launch.sh -t local -p rollbaccine -w 10 -m 2
```

Note that sudo access will be required to mount tmpfs.

Once startup is complete (when the script terminates), you can run TPC-C against postgres with `./benchbase_run.sh`.
Results from running the benchmark can be found in `~/benchbase/target/benchbase-postgres/results`.

If something goes wrong during the run and you'd like to debug the scripts, the files that each executable uses can be found under `build/<executable name>`, such as `build/client0` for the client, `build/replica0` for the first replica, etc.
The log of each executable can be found in `log.txt` within that directory.
The certificates for CCF can be found in `build/workspace/sandbox_common`, and its log can be read from `build/workspace/sandbox_0/out`.

### Running on Azure

Launch the VMs with `./launch.sh` in the section above, with `-t untrusted` or `-t trusted` based on whether you want regular or confidential VMs.
Then, start the actual machines in the following order:
```bash
./run -t <trusted_mode> -p <postgres_mode> -n ccf
./run -t <trusted_mode> -p <postgres_mode> -n replicas -m <tmpfs memory>
./run -t <trusted_mode> -p <postgres_mode> -n clients -w <wait time> -m <tmpfs memory>
./run -t <trusted_mode> -p <postgres_mode> -n benchbase
```

Only the client and benchbase need to be run if `<postgres_mode>` is not `rollbaccine`.
Here's what happens during startup and why the order is important:
1. CCF launches
2. Replicas send their certificates to CCF
3. The client send its certificate to CCF, downloads the certificates of the replicas in its configuration, completes matchmaking with that configuration, then begins leader election with the replicas.
4. Replicas receive the new configuration from the client, query CCF for the replicas' certificates, and connect to each other.
5. Once the client has won leader election, writes can occur (from postgres, for example).

### Cleaning up

Execute the following, adding the `-r` option if running on Azure:
```bash
./cleanup.sh
```

Note that sudo access will be required to unmount tmpfs.


## Developing locally

This section is for testing modifications to the code without using `./launch.sh`.

TODO: Figure of how writes enter through the mountpoint, are sent to the replicas, and then are written to the redirect point and the replica's directory.

The system produces 2 executables: `replica/disk_tees` and `client/tee_fuse`. You can get a rough idea of how to run each one by reading `cloud_scripts/cloud_init/replica_init.sh` and `cloud_scripts/cloud_init/client_init_rollbaccine.sh`. Some of the client's parameters are from libfuse, so I'll document them below, although I've added more parameters.

```bash
client/tee_fuse -f -s <mountpoint>
```
- `-f` states that the client should run in the foreground, so we can see any error logs.
- `-s` turns on single-threaded mode, which is required for sequentially sequencing writes.
- `<mountpoint>` is the directory where the filesystem will be mounted.
To see all the options provided by FUSE, execute `client/tee_fuse -h`.

The system uses CCF to store configurations. The main logic is in `ccf/src/matchmaker.js`.

When you're done running the system, or if the system terminates unexpectedly, you may need to unmount by executing the following:
```bash
sudo umount <mountpoint>
```


## Libraries
This project makes use of (and implicitly trusts) the following libraries:

- [OpenSSL](https://wiki.openssl.org/index.php/Main_Page) - Used for TLS encrypted connections
- [JSON for modern C++](https://github.com/nlohmann/json#examples) - Used for reading JSON config files
- [zpp::bits](https://github.com/eyalz800/zpp_bits) - Used for fast serializing/deserializing of CPP structs. Requires C++20
- [libfuse](https://github.com/libfuse/libfuse) - For intercepting writes to the file system
- [Fusepp](https://github.com/jachappell/Fusepp) - For wrapping FUSE functions in C++. Copied into `client/Fuse.hpp` and `client/Fuse.cpp`.
- [CCF](https://github.com/microsoft/CCF/) - For storing configurations
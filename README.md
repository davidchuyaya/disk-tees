# disk-tees
2f+1 confidential servers to support a rollback-detecting disk abstraction.

## Installation 
Execute the following commands:
```bash
./install.sh
cmake .
make
```

## Launching in Azure
### Untrusted
Execute the following:
```bash
./launch_untrusted.sh
./distribute_cert.sh
```

Note that the script will not work if you are not under the "Azure Research Subs" subscription in Azure. Be sure to modify the script to use your own subscription. More details and configuration parameters can be found at the top of the script.

## Shutting down Azure
### Untrusted
Execute the following:
```bash
./cleanup_untrusted.sh
```
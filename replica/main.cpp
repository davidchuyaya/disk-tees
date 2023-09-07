#include <iostream>
#include <chrono>
#include <variant>
#include <unistd.h>
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/network_config.hpp"
#include "../shared/fuse_messages.hpp"
#include "replica_fuse.hpp"

config parseArgs(int argc, char* argv[]) {
    config conf = {};

    // set default values
    if (argc == 1) {
        std::cerr << "Usage: " << argv[0] << " -i <id> -t <trusted mode> -u <username>" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parse command line agruments
    int opt;
    while ((opt = getopt(argc, argv, "i:t:u:")) != -1) {
        switch (opt) {
            case 'i':
                conf.id = atoi(optarg);
                break;
            case 't':
                conf.trustedMode = optarg;
                break;
            case 'u':
                conf.username = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
        }
    }

    return conf;
}

int main(int argc, char* argv[]) {
    config conf = parseArgs(argc, argv);

    std::string path = "/home/" + conf.username + "/disk-tees/build/replica" + std::to_string(conf.id);
    networkConfig ccfConf = NetworkConfig::readNetworkConfig("ccf.json");
    std::string name = "replica" + std::to_string(conf.id);

    ReplicaFuse replicaFuse(conf, name, path + "/storage", ccfConf, path);
    TLS<clientMsg> clientTLS(conf.id, name, Replica, {}, path, [&](const clientMsg& payload, const std::string& addr) {
       replicaFuse.bufferMsg(payload, addr);
    });
    replicaFuse.addClientTLS(&clientTLS);

    while (true) {
        // Listen forever. Wake at least once every 5000ms in case fsync times out.
        clientTLS.runEventLoopOnce(5000);
        // Process all messages in the queue
        replicaFuse.processPendingMessages();
        replicaFuse.checkFsyncTimeout();
    }

    return 0;
}
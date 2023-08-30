#include <iostream>
#include <chrono>
#include <variant>
#include <unistd.h>
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/network_config.hpp"
#include "../shared/fuse_messages.hpp"
#include "replica_fuse.hpp"

struct config {
    int id;
    std::string trustedMode;
};

config parseArgs(int argc, char* argv[]) {
    config conf = {};

    // set default values
    if (argc == 1) {
        std::cerr << "Usage: " << argv[0] << " -i <id> -t <trusted mode>" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parse command line agruments
    int opt;
    while ((opt = getopt(argc, argv, "i:d:")) != -1) {
        switch (opt) {
            case 'i':
                conf.id = atoi(optarg);
                break;
            case 't':
                conf.trustedMode = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
        }
    }

    return conf;
}

std::string getDir(const config& conf) {
    if (std::string(conf.trustedMode) == "local")
        return "~/disk-tees/build/replica" + std::to_string(conf.id) + "/storage";
    else
        return "/home/azureuser/disk-tees/build/replica" + std::to_string(conf.id) + "/storage";
}

int main(int argc, char* argv[]) {
    config conf = parseArgs(argc, argv);
    
    std::string path = std::filesystem::current_path().string();
    networkConfig ccfConf = NetworkConfig::readNetworkConfig("ccf.json");
    std::string name = "replica" + std::to_string(conf.id);

    ReplicaFuse replicaFuse(conf.id, name, getDir(conf), ccfConf, path);
    TLS<clientMsg, replicaMsg> clientTLS(conf.id, name, Client, Replica, {}, path,
        [&](const clientMsg& payload, const std::string& addr, SSL* sender) {
        std::unique_lock<std::mutex> lock(replicaFuse.allMutex);
        replicaFuse.shouldBroadcast = true; // Broadcast messages we directly received from the client
        // Essentially a switch statement matching the possible types of messages
        std::visit(replicaFuse, payload);
    });
    replicaFuse.addClientTLS(&clientTLS);

    while (true) {}

    return 0;
}
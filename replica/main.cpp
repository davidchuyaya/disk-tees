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
    std::string dir;
};

std::chrono::steady_clock::time_point getTime() {
    return std::chrono::steady_clock::now();
}

int secondsSince(std::chrono::steady_clock::time_point startTime) {
    return std::chrono::duration_cast<std::chrono::seconds>(getTime() - startTime).count();
}

int toMicroseconds(int seconds) {
    return seconds * 1000000;
}

config parseArgs(int argc, char* argv[]) {
    config conf = {};

    // set default values
    if (argc == 1) {
        conf.dir = ".";
        return conf;
    }

    // Parse command line agruments
    int opt;
    while ((opt = getopt(argc, argv, "i:d:")) != -1) {
        switch (opt) {
            case 'i':
                conf.id = atoi(optarg);
                break;
            case 'd': // directory, no ending slash
                conf.dir = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
        }
    }

    return conf;
}

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
int main(int argc, char* argv[]) {
    config conf = parseArgs(argc, argv);
    networkConfig netConf = readNetworkConfig("network.json");

    ReplicaFuse replicaFuse;
    TLS<clientMsg, diskTeePayload> replicaTLS(conf.id, netConf.replicas, [&](clientMsg payload, SSL* sender) {
        // Essentially a switch statement matching the possible types of messages
        std::visit(replicaFuse, payload.params);
    });

    while (true) {}

    // while (true) {
    //     std::string input;
    //     std::getline(std::cin, input);
    //     diskTeePayload sendPayload = { .data = input };
    //     replicaTLS.broadcast(sendPayload);
    // }

    return 0;
}
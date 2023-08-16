#include <iostream>
#include <chrono>
#include <unistd.h>
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/network_config.hpp"

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

int main(int argc, char* argv[]) {
    config conf = parseArgs(argc, argv);
    networkConfig netConf = readNetworkConfig("network.json");

    TLS<diskTeePayload, diskTeePayload> replicaTLS(conf.id, netConf.replicas, [](diskTeePayload payload, SSL* sender) {
        std::cout << "Received message from client: " << payload.data << std::endl;
    });

    while (true) {
        std::string input;
        std::getline(std::cin, input);
        diskTeePayload sendPayload = { .data = input };
        replicaTLS.broadcast(sendPayload);
    }

    return 0;
}
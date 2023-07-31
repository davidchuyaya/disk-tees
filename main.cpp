#include <iostream>
#include <chrono>
#include <unistd.h>
#include "tls.hpp"

struct config {
    bool isServer;
    int port;
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

    // TODO: set default values
    if (argc == 1) {
        return conf;
    }

    // TODO: Parse command line agruments
    int opt;
    while ((opt = getopt(argc, argv, "sp:")) != -1) {
        switch (opt) {
            case 's':
                conf.isServer = true;
                break;
            case 'p':
                conf.port = atoi(optarg);
                break;
            case '?':
                exit(EXIT_FAILURE);
        }
    }

    return conf;
}

int main(int argc, char* argv[]) {
    config conf = parseArgs(argc, argv);

    if (conf.isServer) {
        std::vector<sockaddr_in> peers;
        TLS tls(peers, conf.port, [](clientInPayload payload) {
            std::cout << "Received message from client" << std::endl;
        }, [](diskTeePayload payload) {
            std::cout << "Received message from disk tee" << std::endl;
        });
    }
    else {
        runClient(conf.port);
    }

    return 0;
}
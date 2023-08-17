#include "client_fuse.hpp"
#include "../shared/network_config.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"

int main(int argc, char *argv[]) {
    // For some reason, the client's current path changes to root between the reading of network config and TLS, so we pass it to TLS
    const std::string& path = std::filesystem::current_path().string();

    std::cout << "Looking for network config in path: " << path << std::endl;
    networkConfig netConf = readNetworkConfig("network.json");

    // TODO: Remove hardcoded ID 1
    std::filesystem::current_path(path);
    TLS<diskTeePayload, clientMsg> replicaTLS(1, netConf.replicas, path, [](diskTeePayload payload, SSL* sender) {
        std::cout << "Received message from replica: " << payload.data << std::endl;
    });

    ClientFuse fuse(&replicaTLS);
    return fuse.run(argc, argv);
}
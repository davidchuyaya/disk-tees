#include "client_fuse.hpp"
#include "../shared/network_config.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"

int main(int argc, char *argv[]) {
    networkConfig netConf = readNetworkConfig("network.json");

    TLS<diskTeePayload, clientMsg> replicaTLS(1, netConf.replicas, [](diskTeePayload payload, SSL* sender) {
        std::cout << "Received message from replica: " << payload.data << std::endl;
    });

    ClientFuse fuse(&replicaTLS);
    return fuse.run(argc, argv);
}
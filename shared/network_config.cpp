#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "network_config.hpp"

networkConfig readNetworkConfig(const std::string& file) {
    std::ifstream f(file);
    json data = json::parse(f);
    
    networkConfig out;

    for (auto& e : data) {
        int id = e["id"];
        peer addr = {
            .ip = e["ip"],
            .name = e["name"]
        };
        peer replicaAddr = addr;
        replicaAddr.port = e["replicaPort"];
        out.replicas.insert(out.replicas.begin() + id, replicaAddr);
        peer clientAddr = addr;
        clientAddr.port = e["clientPort"];
        out.clients.insert(out.clients.begin() + id, clientAddr);
        peer ccfAddr = addr;
        ccfAddr.port = e["ccfPort"];
        out.ccfNodes.insert(out.ccfNodes.begin() + id, ccfAddr);
    }

    return out;
}
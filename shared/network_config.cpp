#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "network_config.hpp"

networkConfig NetworkConfig::readNetworkConfig(const std::string& file) {
    std::ifstream f(file);
    json data = json::parse(f);
    
    networkConfig out;
    for (auto& e : data)
        out[e["id"]] = e["ip"];
    return out;
}

addresses NetworkConfig::configToAddrs(const NodeType type, const networkConfig& conf) {
    addresses out;
    for (auto& [id, ip] : conf)
        out.emplace_back(ip + ":" + std::to_string(getPort(type, id)));
    return out;
}

int NetworkConfig::getPort(const NodeType type, const int id) {
    return type == NodeType::Client ? CLIENT_START_PORT + id : REPLICA_START_PORT + id;
}
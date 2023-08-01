/**
 * Read network configurations from a JSON file.
*/

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include "network_config.hpp"
using json = nlohmann::json;

/**
 * Expected JSON format:
 * [
 *   {
 *     "ip": "10.0.0.4",
 *     "port": 8080,
 *     "name": "disk_tee0", // Certificate should be disk_tee0.pem
 *     "id": 0 // 0-indexed
 *   }
 * ]
*/
networkConfig readNetworkConfig(const std::string file) {
    std::ifstream f(file);
    json data = json::parse(f);
    
    networkConfig out;
    out.peers.reserve(data.size());
    out.names.reserve(data.size());

    for (auto& e : data) {
        int id = e["id"];

        // set IP address
        std::string ip = e["ip"];
        int port = e["port"];
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);   
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr.s_addr));     
        out.peers.insert(out.peers.begin() + id, addr);

        // set certificate file name
        std::string name = e["name"];
        out.names.insert(out.names.begin() + id, name);
    }

    return out;
}
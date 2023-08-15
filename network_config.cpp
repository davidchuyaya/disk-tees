#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "network_config.hpp"

/**
 * Expected JSON format:
 * [
 *   {
 *     "ip": "127.0.0.1",
 *     "port": 4433,
 *     "name": "disk_tee0", // Certificate should be disk_tee0.pem
 *     "id": 0 // 0-indexed
 *   }
 * ]
*/
std::vector<networkConfig> readNetworkConfig(const std::string& file) {
    std::ifstream f(file);
    json data = json::parse(f);
    
    std::vector<networkConfig> out;
    out.reserve(data.size());

    for (auto& e : data) {
        int id = e["id"];
        networkConfig conf = {
            .ip = e["ip"],
            .port = e["port"],
            .name = e["name"]
        };
        out.insert(out.begin() + id, conf);
    }

    return out;
}
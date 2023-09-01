#include <sstream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <ranges>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "ccf_network.hpp"

const int SLEEP_SECONDS = 5;
const std::string GET_CERT_OUTPUT_FILE = "get_cert.json";
const std::string MATCH_OUTPUT_FILE = "match.json";
const std::string GARBAGE_OUTPUT_FILE = "garbage.json";

CCFNetwork::CCFNetwork(const std::string& path, const networkConfig ccf) : path(path) {
    auto valuesRange = std::views::values(ccf);
    ccfAddresses = std::vector<std::string>(valuesRange.begin(), valuesRange.end());
    liveAddresses = ccfAddresses;
}

void CCFNetwork::getCert(const int id, const std::string& name, const std::string& type) {
    while (true) {
        removeOutputFile(GET_CERT_OUTPUT_FILE);

        // construct bash command
        const std::string& address = liveAddresses.back();
        std::ostringstream ss;
        ss << path << "../../cloud_scripts/ccf_networking/get_cert_no_retry.sh";
        ss << " -a " << address;
        ss << " -i " << id;
        ss << " -n " << name;
        ss << " -t " << type;
        std::cout << "Executing command: " << ss.str() << std::endl;
        int errorCode = system(ss.str().c_str());

        if (curlSuccessful(errorCode) && requestSuccessful(GET_CERT_OUTPUT_FILE))
            break;
    }
    removeOutputFile(GET_CERT_OUTPUT_FILE);
}

matchB CCFNetwork::sendMatchA(const ballot r, const networkConfig config) {
    // Construct JSON
    json sendData = {
        {"ballot", ballotToString(r)},
        {"config", config}
    };

    while (true) {
        removeOutputFile(MATCH_OUTPUT_FILE);

        // construct bash command
        const std::string& address = liveAddresses.back();
        std::ostringstream ss;
        ss << path << "../../cloud_scripts/ccf_networking/send_json.sh";
        ss << " -a " << address;
        ss << " -e match";
        ss << " -s "<< sendData.dump();
        std::cout << "Executing command: " << ss.str() << std::endl;
        int errorCode = system(ss.str().c_str());

        if (curlSuccessful(errorCode) && requestSuccessful(GET_CERT_OUTPUT_FILE))
            break;
    }

    // Read json, un-stringify config (returned as array of configs encoded as strings)
    matchB output;
    std::ifstream file;
    file.exceptions(std::ifstream::badbit);
    try {
        file.open(MATCH_OUTPUT_FILE);
        json data = json::parse(file);
        output.r = ballotFromString(data["ballot"]);
        for (const std::string& configString : data["config"]) {
            json config = json::parse(configString);
            output.configs.emplace_back(config);
        }
    } catch (std::ifstream::failure e) {
        std::cout << "Error opening file " << MATCH_OUTPUT_FILE << std::endl;
        exit(EXIT_FAILURE);
    } catch (json::exception e) {
        std::cout << "Error reading JSON " << MATCH_OUTPUT_FILE << std::endl;
        exit(EXIT_FAILURE);
    }
    removeOutputFile(MATCH_OUTPUT_FILE);

    return output;
}

void CCFNetwork::sendGarbageA(const ballot r) {
    // Construct JSON
    json sendData = {
        {"ballot", ballotToString(r)}
    };

    while (true) {
        removeOutputFile(GARBAGE_OUTPUT_FILE);

        // construct bash command
        const std::string& address = liveAddresses.back();
        std::ostringstream ss;
        ss << path << "../../cloud_scripts/ccf_networking/send_json.sh";
        ss << " -a " << address;
        ss << " -e garbage";
        ss << " -s "<< sendData.dump();
        std::cout << "Executing command: " << ss.str() << std::endl;
        int errorCode = system(ss.str().c_str());

        if (curlSuccessful(errorCode) && requestSuccessful(GET_CERT_OUTPUT_FILE))
            break;
    } 
    removeOutputFile(MATCH_OUTPUT_FILE);
}

std::string CCFNetwork::ballotToString(const ballot& r) {
    std::ostringstream ss;
    ss << r.ballotNum << "," << r.clientId << "," << r.configNum;
    return ss.str();
}

ballot CCFNetwork::ballotFromString(const std::string& s) {
    std::istringstream ss(s);
    std::string token;
    std::getline(ss, token, ',');
    int ballotNum = std::stoi(token);
    std::getline(ss, token, ',');
    int clientId = std::stoi(token);
    std::getline(ss, token, ',');
    int configNum = std::stoi(token);
    return {ballotNum, clientId, configNum};
}

void CCFNetwork::removeOutputFile(const std::string& outputFile) {
    std::remove(outputFile.c_str());
}

bool CCFNetwork::curlSuccessful(const int errorCode) {
    if (errorCode == 0)
        return true;
    std::cout << "Curl for address " << liveAddresses.back() << " failed with error code " << errorCode << std::endl;
    markLastAddressDead();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    return false;
}

bool CCFNetwork::requestSuccessful(const std::string& outputFile) {
    std::ifstream file;
    file.exceptions(std::ifstream::badbit);
    try {
        file.open(outputFile);
        json data = json::parse(file);
        bool success = data["success"];
        if (!success)
            std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
        return success;
    } catch (std::ifstream::failure e) {
        std::cout << "Error opening file " << outputFile << std::endl;
        return false;
    } catch (json::exception e) {
        std::cout << "Error reading JSON " << outputFile << std::endl;
        return false;
    }
}

void CCFNetwork::markLastAddressDead() {
    liveAddresses.pop_back();
    // Reset if all CCF nodes have been marked dead, since "marking dead" is just a liveness optimization
    if (liveAddresses.empty()) {
        liveAddresses = ccfAddresses;
    }
}
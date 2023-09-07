#include <chrono>
#include <thread>
#include "client_fuse.hpp"
#include "../shared/network_config.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"
#include "../shared/ccf_network.hpp"

#define DISK_REQ_TIMEOUT 60 * 1000 // 60 seconds

static struct config {
    int id;
    bool network;
    const char* trustedMode;
    const char* username;
} config;

// Fuse parsing logic from: https://github.com/libfuse/libfuse/blob/master/example/hello.c
#define OPTION(t, p) { t, offsetof(struct config, p), 1 }
static const struct fuse_opt config_spec[] = {
	OPTION("-i %d", id),
    OPTION("-t %s", trustedMode),
    OPTION("-u %s", username),
    OPTION("-n", network),
	FUSE_OPT_END
};

matchB matchmake(CCFNetwork& ccfNetwork, const int id, const networkConfig& replicaConf) {
    ballot r {
        .ballotNum = 0,
        .clientId = id,
        .configNum = 0
    };
    while (true) {
        matchB reply = ccfNetwork.sendMatchA(r, replicaConf);
        if (reply.r == r) {
            return reply;
        }
        // Retry with a higher ballot number
        r.ballotNum = reply.r.ballotNum + 1;
    }
}

int main(int argc, char* argv[]) {
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &config, config_spec, NULL) == 1)
        exit(EXIT_FAILURE);

    std::string path = "/home/" + std::string(config.username) + "/disk-tees/build/client" + std::to_string(config.id);

    // No replicas
    if (!config.network) {
        ClientFuse fuse(config.network, ballot {0, 0, 0}, path + "/storage", 0, {});
        return fuse.run(args.argc, args.argv);
    }
    // Otherwise, begin leader election

    networkConfig ccfConf = NetworkConfig::readNetworkConfig("ccf.json");
    networkConfig replicaConf = NetworkConfig::readNetworkConfig("replicas.json");

    // 1. Send MatchA to CCF, wait for MatchB
    std::cout << "Beginning matchmaking phase" << std::endl;
    CCFNetwork ccfNetwork(path, ccfConf);
    matchB matchmakeResult = matchmake(ccfNetwork, config.id, ccfConf);
    ballot r = matchmakeResult.r;
    std::cout << "Matchmaking complete" << std::endl;

    // 2. Find the replica addresses for the current config
    addresses replicas = NetworkConfig::configToAddrs(Replica, replicaConf);
    int quorum = replicaConf.size() / 2 + 1;
    std::string name = "client" + std::to_string(config.id);

    // 3. Find all replicas in past or present configs, removing duplicates
    matchmakeResult.configs.emplace_back(replicaConf);
    std::map<int, std::string> allReplicasConf;
    for (const auto& netConf : matchmakeResult.configs)
        allReplicasConf.insert(netConf.begin(), netConf.end());
    addresses allReplicas = NetworkConfig::configToAddrs(Replica, allReplicasConf);
    
    // 4. Connect to all replicas
    std::cout << "Connecting to replicas" << std::endl;
    ClientFuse fuse(config.network, r, path + "/storage", quorum, replicas);
    TLS<replicaMsg> replicaTLS(config.id, name, Client, allReplicasConf, path,
        [&](const replicaMsg& payload, const std::string& addr) {
            fuse.sender = addr;
            std::visit(fuse, payload);
    });
    std::cout << "Connected to replicas" << std::endl;
    fuse.addTLS(&replicaTLS);

    // 5. Broadcast p1as
    replicaTLS.broadcast<clientMsg>(p1a { 
        .seq = 0,
        .r = r,
        .netConf = replicaConf
    }, allReplicas);

    // 6. Process p1bs
    std::cout << "Broadcasted p1a to replicas, waiting for p1b quorum" << std::endl;
    while (!fuse.p1bQuorum(matchmakeResult.configs)) {
        replicaTLS.runEventLoopOnce(-1);
    }

    // 7. Send disk request to highest ranking replica, if it has disk. Retry on timeout
    bool noPriorState = false;
    disk diskMsg;
    while (true) {
        p1b highestRanking = fuse.highestRankingP1b();
        if (highestRanking.written < 0) {
            noPriorState = true;
            std::cout << "No prior state, don't need to request disk" << std::endl;
            break;
        }

        std::cout << "P1b quorum reached, requesting disk from replica: " << highestRanking.id << std::endl;
        replicaTLS.send<clientMsg>(diskReq{.seq = 0, .id = config.id, .r = r},
                                   allReplicasConf[highestRanking.id]);

        // Wait for disk response until timeout
        auto begin = std::chrono::steady_clock::now();
        while (fuse.diskChecksums.empty()) {
            replicaTLS.runEventLoopOnce(DISK_REQ_TIMEOUT);
            auto end = std::chrono::steady_clock::now();

            // timed out
            if (std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() >= DISK_REQ_TIMEOUT)
                break;
        }

        // Attempt to read any disk that is available
        while (true) {
            bool copiedDisk = false;
            for (const auto& disk : fuse.diskChecksums) {
                std::ostringstream ss;
                ss << path << "/../../cloud_scripts/unzip_disk.sh";
                ss << " -s replica" << disk.id;
                ss << " -d " << name;
                ss << " -c " << disk.diskChecksum;
                ss << " -u " << config.username;
                std::cout << "Executing command: " << ss.str() << std::endl;
                int errorCode = system(ss.str().c_str());

                if (errorCode == 0) {
                    copiedDisk = true;
                    fuse.written = disk.written;
                    diskMsg = disk;
                    std::cout << "Copied disk from replica" << std::endl;
                    break;
                }
            }
            if (copiedDisk)
                break;

            // If no disk exists, see if there's another replica we can try
            fuse.successfulP1bs.erase(highestRanking);
            if (fuse.p1bQuorum(matchmakeResult.configs))
                break;
            else { // No other replica to try, keep waiting
                fuse.successfulP1bs.emplace(highestRanking);
                replicaTLS.runEventLoopOnce(-1);
            }
        }
    }

    // 8. Rebroadcast disk (p2a)
    if (!noPriorState) {
        std::cout << "Rebroadcasting disk" << std::endl;
        std::string replicaName = "replica" + std::to_string(diskMsg.id);
        for (auto& [id, ip] : replicaConf) {
            if (id != diskMsg.id) {
                // Send the disk
                std::ostringstream ss;
                ss << path << "/../../cloud_scripts/send_disk.sh";
                ss << " -s " << replicaName;
                ss << " -d replica" << id;
                ss << " -a " << ip << ":" << NetworkConfig::getPort(Replica, id);
                ss << " -u " << config.username;
                ss << " -t " << config.trustedMode; // Note: Excludes -z because we're just sending the already-zipped disk, don't need to rezip
                std::cout << "Executing command: " << ss.str() << std::endl;
                int errorCode = system(ss.str().c_str());
            }
        }
        // Delete zipped disk
        std::remove((path + "/zipped-storage/" + replicaName + ".tar.gz").c_str());
    }
    fuse.successfulP1bs.clear();
    fuse.diskChecksums.clear();

    std::cout << "Broadcasting p2a to replicas" << std::endl;
    replicaTLS.broadcast<clientMsg>(p2a {
        .seq = 0,
        .r = r,
        .written = fuse.written,
        .diskReplicaName = noPriorState ? "" : "replica" + std::to_string(diskMsg.id),
        .diskChecksum = noPriorState ? "" : diskMsg.diskChecksum
    }, replicas);
    while (fuse.successfulP2bs.size() < quorum) {
        replicaTLS.runEventLoopOnce(-1);
    }
    std::cout << "P2b quorum reached, leader election complete!" << std::endl;

    // 9. Kill old replicas, if there are any
    if (allReplicasConf.size() > replicaConf.size()) {
        ccfNetwork.sendGarbageA(r);
        // Note: We can't actually kill replicas from here. Rely on trusted admin once they see the replica's no longer in the config.
    }

    return fuse.run(args.argc, args.argv);
}
#include "client_fuse.hpp"
#include "../shared/network_config.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"
#include "../shared/ccf_network.hpp"

static struct config {
    int id;
    bool network;
    const char* trustedMode;
    const char* installScript;
    const char* runScript;
} config;

// Fuse parsing logic from: https://github.com/libfuse/libfuse/blob/master/example/hello.c
#define OPTION(t, p) { t, offsetof(struct config, p), 1 }
static const struct fuse_opt config_spec[] = {
	OPTION("-i %d", id),
    OPTION("-t %s", trustedMode),
    OPTION("-n", network),
    OPTION("--install=%s", installScript),
    OPTION("--run=%s", runScript),
	FUSE_OPT_END
};

std::string getPath() {
    if (std::string(config.trustedMode) == "local")
        return std::string(getenv("HOME")) + "/disk-tees/build/client" + std::to_string(config.id);
    else
        return "/home/azureuser/disk-tees/build/client" + std::to_string(config.id);
}

matchB matchmake(const int id, const networkConfig& ccfConf, const networkConfig& replicaConf) {
    ballot r {
        .ballotNum = 0,
        .clientId = id,
        .configNum = 0
    };
    CCFNetwork ccfNetwork(ccfConf);
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

    // No replicas
    // if (!config.network) {
    //     ClientFuse fuse(config.network, ballot {0, 0, 0}, path + "/storage", 0, {});
    //     return fuse.run(args.argc, args.argv);
    // }
    // // Otherwise, begin leader election

    // // For some reason, the client's current path changes to root between the reading of network config and TLS, so we pass it to TLS
    std::string path = getPath();

    // networkConfig ccfConf = NetworkConfig::readNetworkConfig("ccf.json");
    // networkConfig replicaConf = NetworkConfig::readNetworkConfig("replicas.json");

    // // 1. Send MatchA to CCF, wait for MatchB
    // matchB matchmakeResult = matchmake(config.id, ccfConf, replicaConf);
    // // 2. Connect to replicas
    // addresses replicas = NetworkConfig::configToAddrs(Replica, replicaConf);
    // int quorum = replicaConf.size() / 2 + 1;
    std::string name = "client" + std::to_string(config.id);
    
    // ClientFuse fuse(config.network, matchmakeResult.r, path + "/storage", quorum, replicas);
    // TLS<replicaMsg, clientMsg> replicaTLS(config.id, name, Replica, Client, replicaConf, path,
    //     [&](const replicaMsg& payload, const std::string& addr, SSL* sender) {
    //         std::visit(fuse, payload);
    // });
    // fuse.addTLS(&replicaTLS);
    // // 3. Broadcast p1as
    // replicaTLS.broadcast(p1a { 
    //     .r = matchmakeResult.r,
    //     .netConf = replicaConf
    // }, replicas);
    // // 4. Process p1bs
    // // TODO: Have replicas respond

    // // TODO: Skip networking if it is turned off, replace network macro
    // // TODO: If there is no prior state, run file setup script
    // // TODO: Run start script

    TLS<helloMsg, helloMsg> replicaTLS(config.id, name, Replica, Client, {{1, "127.0.0.1"}}, path,
        [&](const helloMsg& payload, const std::string& addr, SSL* sender) {
            std::cout << "Received message from replica: " << payload.text << std::endl;
    });


    while (true) {
        for (std::string line; std::getline(std::cin, line);) {
            replicaTLS.broadcast(helloMsg {
                .text = line
            });
        }
        replicaTLS.runEventLoopOnce(-1);
    }
    
    // return fuse.run(args.argc, args.argv);
}
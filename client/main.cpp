#include "client_fuse.hpp"
#include "../shared/network_config.hpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"

static struct config {
    int id;
    const char* redirectPoint;
} config;

// Fuse parsing logic from: https://github.com/libfuse/libfuse/blob/master/example/hello.c
#define OPTION(t, p) { t, offsetof(struct config, p), 1 }
static const struct fuse_opt config_spec[] = {
	OPTION("-i %d", id),
    OPTION("-r %s", redirectPoint),
	FUSE_OPT_END
};

int main(int argc, char* argv[]) {
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &config, config_spec, NULL) == 1)
        exit(EXIT_FAILURE);
    
    // For some reason, the client's current path changes to root between the reading of network config and TLS, so we pass it to TLS
    std::string path = std::filesystem::current_path().string();

    networkConfig netConf = readNetworkConfig("network.json");

    // TODO: Change quorum to f+1 number of nodes
    ClientFuse fuse(config.redirectPoint, 1);
    std::filesystem::current_path(path);
    TLS<replicaMsg, clientMsg> replicaTLS(config.id, netConf.replicas, path, [&](replicaMsg payload, SSL* sender) {
        fuse.onRecvMsg(payload, sender);
    });
    fuse.addTLS(&replicaTLS);
    
    return fuse.run(args.argc, args.argv);
}
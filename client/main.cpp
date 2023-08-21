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

    std::filesystem::current_path(path);
    TLS<diskTeePayload, clientMsg> replicaTLS(config.id, netConf.replicas, path, [](diskTeePayload payload, SSL* sender) {
        std::cout << "Received message from replica: " << payload.data << std::endl;
    });

    ClientFuse fuse(&replicaTLS, config.redirectPoint);
    return fuse.run(args.argc, args.argv);
}
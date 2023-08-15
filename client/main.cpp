#define FUSE_USE_VERSION 31
#include <fuse.h>

static const struct fuse_operations xmp_oper = {};

int main(int argc, char* argv[]) {
    fuse_main(argc, argv, &xmp_oper, NULL);
    return 0;
}
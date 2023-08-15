/**
 * Logic mostly copied from https://github.com/davidchuyaya/TEE-fuse/blob/master/example/passthrough_fh.c
*/
#include <sys/file.h>
#include "fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define CUSTOM_DIR
#ifdef CUSTOM_DIR
#define PREPEND_PATH(path) ("/home/davidchuyaya/disk-tees-test" + path)
#elif
#define PREPEND_PATH(path) (path)
#endif

void Fuse::create(const createParams& params) {
    int fd = open(PREPEND_PATH(params.path).c_str(), params.fi.flags, params.mode);
    fileHandleConverter[params.fi.fh] = fd;
}
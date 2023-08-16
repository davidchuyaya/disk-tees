/**
 * Logic mostly copied from https://github.com/davidchuyaya/TEE-fuse/blob/master/example/passthrough_fh.c
*/
#include <sys/file.h>
#include <iostream>
#include "replica_fuse.hpp"

// If defined, will write files to the directory specified below. Used for testing locally.
#define CUSTOM_DIR
#ifdef CUSTOM_DIR
#define PREPEND_PATH(path) (("/home/davidchuyaya/disk-tees-test" + std::string(path)).c_str())
#elif
#define PREPEND_PATH(path) (path)
#endif

void ReplicaFuse::operator()(const truncateParams& params) {
    std::cout << "Received truncate from client" << std::endl;
    // int fd = open(PREPEND_PATH(params.path), params.fi.flags, params.mode);
    // fileHandleConverter[params.fi.fh] = fd;
}

void ReplicaFuse::operator()(const createParams& params) {
    std::cout << "Received create from client on path: " << params.path << std::endl;
    // int fd = open(PREPEND_PATH(params.path), params.fi.flags, params.mode);
    // fileHandleConverter[params.fi.fh] = fd;
}

void ReplicaFuse::operator()(const writeBufParams& params) {
    std::cout << "Received writeBuf from client" << std::endl;
}

void ReplicaFuse::operator()(const fsyncParams& params) {
    std::cout << "Received fsync from client" << std::endl;
    // int fd = open(PREPEND_PATH(params.path), params.fi.flags, params.mode);
    // fileHandleConverter[params.fi.fh] = fd;
}

void ReplicaFuse::operator()(const releaseParams& params) {
    std::cout << "Received release from client" << std::endl;
    // int fd = open(PREPEND_PATH(params.path), params.fi.flags, params.mode);
    // fileHandleConverter[params.fi.fh] = fd;
}
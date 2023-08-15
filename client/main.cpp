#include "tee_fuse.hpp"

int main(int argc, char *argv[])
{
    TeeFuse fuse;
    return fuse.run(argc, argv);
}
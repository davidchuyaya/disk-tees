#pragma once
#include <map>
#include <set>
#include "Fuse.hpp"
#include "Fuse.cpp"
#include "../shared/tls.hpp"
#include "../shared/tls.cpp"
#include "../shared/fuse_messages.hpp"

/**
 * Implementation of Fuse, based on https://github.com/libfuse/libfuse/blob/master/example/passthrough_fh.c
 */
class ClientFuse : public Fusepp::Fuse<ClientFuse>
{
public:
    ClientFuse(const bool network, const ballot& r, const std::string& redirectPoint, const int quorum, const addresses& replicas);
    void addTLS(TLS<replicaMsg> *tls);
    bool p1bQuorum(const std::vector<networkConfig>& configs);
    p1b highestRankingP1b();

    // Visitor pattern: https://www.cppstories.com/2018/09/visit-variants/. One for every possible type in replicaMsg
    void operator()(const p1b& msg); 
    void operator()(const disk& msg);
    void operator()(const p2b& msg); 
    void operator()(const fsyncMissing& msg);
    void operator()(const fsyncAck& msg);
    // TODO: Open port for user, listen for reconfiguration messages

    // FUSE functions
    static void *client_init(fuse_conn_info *conn, fuse_config *cfg);
    static int client_getattr(const char *path, struct stat *stbuf, fuse_file_info *fi);
    static int client_access(const char *path, int mask);
    static int client_readlink(const char *path, char *buf, size_t size);
    static int client_opendir(const char *path, fuse_file_info *fi);
    static int client_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi, enum fuse_readdir_flags flags);
    static int client_releasedir(const char *path, fuse_file_info *fi);
    // static int client_read(const char *path, char *buf, size_t size, off_t offset, fuse_file_info *fi); // Don't support bc read_buf overrides it
    static int client_read_buf(const char *path, fuse_bufvec **bufp, size_t size, off_t offset, fuse_file_info *fi);
    static int client_statfs(const char *path, struct statvfs *stbuf);
    static int client_flush(const char *path, fuse_file_info *fi);
    static int client_getxattr(const char *path, const char *name, char *value, size_t size);
    static int client_listxattr(const char *path, char *list, size_t size);
    // static int client_lock(const char *path, fuse_file_info *fi, int cmd, struct flock *lock); // Don't support bc there's no ulockmgr library in Linux
    static int client_flock(const char *path, fuse_file_info *fi, int op);
    static int client_lseek(const char *path, off_t off, int whence, fuse_file_info *fi);

    /**
     * The following functions require communication with the replicas.
     */
    static int client_mknod(const char *path, mode_t mode, dev_t rdev);
    static int client_mkdir(const char *path, mode_t mode);
    static int client_symlink(const char *target, const char *linkpath);
    static int client_unlink(const char *path);
    static int client_rmdir(const char *path);
    static int client_rename(const char *oldpath, const char *newpath, unsigned int flags);
    static int client_link(const char *oldpath, const char *newpath);
    static int client_chmod(const char *path, mode_t mode, fuse_file_info *fi);
    static int client_chown(const char *path, uid_t uid, gid_t gid, fuse_file_info *fi);
    static int client_truncate(const char *path, off_t offset, fuse_file_info *fi);
    static int client_utimens(const char *path, const timespec tv[2], fuse_file_info *fi);
    static int client_create(const char *path, mode_t mode, fuse_file_info *fi);
    static int client_open(const char *path, fuse_file_info *fi);
    // static int client_write(const char *path, const char *buf, size_t size, off_t offset, fuse_file_info *fi); // Don't support bc write_buf overrides it
    static int client_write_buf(const char *path, fuse_bufvec *buf, off_t offset, fuse_file_info *fi);
    static int client_release(const char *path, fuse_file_info *fi);
    static int client_fsync(const char *path, int isdatasync, fuse_file_info *fi);
    static int client_fallocate(const char *path, int mode, off_t offset, off_t length, fuse_file_info *fi);
    static int client_setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
    static int client_removexattr(const char *path, const char *name);
    static int client_copy_file_range(const char *path_in, fuse_file_info *fi_in, off_t off_in, const char *path_out, fuse_file_info *fi_out, off_t off_out, size_t len, int flags);

    inline static std::string sender; // Set before each visitor function is called
    inline static int written = -1;
    inline static int replicasCommitted = 0;  // The largest sequence number for which replicas committed
    // State that indicates where in the protocol we are
    inline static std::set<p1b> successfulP1bs = {};
    inline static std::set<disk> diskChecksums = {};
    inline static std::set<p2b> successfulP2bs = {};
    inline static bool isReconfiguring = false;

private:
    // Because all functions are static, ClientFuse can't carry state except by having static variables
    // inline static: See https://stackoverflow.com/a/62915890/4028758
    // 1. Constants
    inline static bool network;
    inline static int quorum;
    inline static TLS<replicaMsg> *replicaTLS;
    inline static std::string redirectPoint; // Directory to redirect writes to. Since we're passing through all disk operations, this is necessary to prevent retriggering FUSE in the same directory
    inline static ballot r;
    // 2. Mutable state
    inline static std::map<int, clientMsg> uncommittedWrites = {}; // Key = sequence number
    inline static std::map<int, int> replicaWritten = {}; // Map from ID to largest sequence number for each replica
    inline static addresses replicas;
    // 3. State only relevant during specific phases of the protocol
    
    static fuse_file_info_lite make_lite(fuse_file_info *fi);
};
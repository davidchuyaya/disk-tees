#pragma once
#include <map>
#include <condition_variable>
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
    ClientFuse(const std::string& redirectPoint, const int quorum);
    void addTLS(TLS<replicaMsg, clientMsg> *tls);
    void onRecvMsg(const replicaMsg& msg, SSL* sender);

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

private:
    // Because all functions are static, ClientFuse can't carry state except by having static variables
    // inline static: See https://stackoverflow.com/a/62915890/4028758
    inline static TLS<replicaMsg, clientMsg> *replicaTLS;
    inline static int written;
    inline static round r;
    // Directory to redirect writes to. Since we're passing through all disk operations, this is necessary to prevent retriggering FUSE in the same directory
    inline static std::string redirectPoint;
    inline static std::mutex replicaRecvMutex;
    inline static int replicasCommitted;
    //TODO: Clear on reconfiguration. Also needs to pause all other writes on reconfiguration
    inline static std::map<int, int> replicaWritten;
    inline static int quorum;
    inline static std::mutex fsyncMutex;
    inline static std::condition_variable fsyncCommitted;

    static fuse_file_info_lite make_lite(fuse_file_info *fi);
};
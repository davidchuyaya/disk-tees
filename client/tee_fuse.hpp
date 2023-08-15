#pragma once
#include "Fuse.h"
#include "Fuse.cpp"

class TeeFuse : public Fusepp::Fuse<TeeFuse> {
public:
    TeeFuse() {}
    ~TeeFuse() {}

    static void* client_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
    static int client_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
    static int client_access(const char *path, int mask);
    // static int client_readlink(const char *path, char *buf, size_t size);
    static int client_opendir(const char *path, struct fuse_file_info *fi);
    // static int client_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
    static int client_releasedir(const char *path, struct fuse_file_info *fi);
    // static int client_mknod(const char *path, mode_t mode, dev_t rdev);
    // static int client_mkdir(const char *path, mode_t mode);
    // static int client_symlink(const char *target, const char *linkpath);
    static int client_unlink(const char *path);
    // static int client_rmdir(const char *path);
    // static int client_rename(const char *oldpath, const char *newpath, unsigned int flags);
    // static int client_link(const char *oldpath, const char *newpath);
    // static int client_chmod(const char *path, mode_t mode, struct fuse_file_info *fi);
    // static int client_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
    static int client_truncate(const char *path, off_t offset, struct fuse_file_info *fi);
    // static int client_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);
    static int client_create(const char *path, mode_t mode, struct fuse_file_info *fi);
    // static int client_open(const char *path, struct fuse_file_info *fi);
    // static int client_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    // static int client_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t offset, struct fuse_file_info *fi);
    // static int client_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    static int client_write_buf(const char *path, struct fuse_bufvec *buf, off_t offset, struct fuse_file_info *fi);
    // static int client_statfs(const char *path, struct statvfs *stbuf);
    static int client_flush(const char *path, struct fuse_file_info *fi);
    static int client_release(const char *path, struct fuse_file_info *fi);
    static int client_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
    // static int client_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi);
    // static int client_setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
    // static int client_getxattr(const char *path, const char *name, char *value, size_t size);
    // static int client_listxattr(const char *path, char *list, size_t size);
    // static int client_removexattr(const char *path, const char *name);
    // static int client_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock);
    // static int client_flock(const char *path, struct fuse_file_info *fi, int op);
    // static int client_copy_file_range(const char *path_in, struct fuse_file_info *fi_in, off_t off_in, const char *path_out, struct fuse_file_info *fi_out, off_t off_out, size_t len, int flags);
    // static int client_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi);
};
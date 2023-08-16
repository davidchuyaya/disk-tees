//==================================================================
/**
 *  Mostly copied from https://github.com/jachappell/Fusepp/tree/master.
 *  Couldn't figure out how to import with FetchContent.
 * 
 *  FuseApp -- A simple C++ wrapper for the FUSE filesystem
 *
 *  Copyright (C) 2021 by James A. Chappell (rlrrlrll@gmail.com)
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use,
 *  copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following
 *  condition:
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *  OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __FUSE_APP_H__
#define __FUSE_APP_H__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif

#include <fuse.h>
#include <cstring>

namespace Fusepp
{
  typedef int(*t_getattr)(const char *, struct stat *, struct fuse_file_info *);
  typedef int(*t_readlink)(const char *, char *, size_t);
  typedef int(*t_mknod) (const char *, mode_t, dev_t);
  typedef int(*t_mkdir) (const char *, mode_t);
  typedef int(*t_unlink) (const char *);
  typedef int(*t_rmdir) (const char *);
  typedef int(*t_symlink) (const char *, const char *);
  typedef int(*t_rename) (const char *, const char *,  unsigned int);
  typedef int(*t_link) (const char *, const char *);
  typedef int(*t_chmod) (const char *, mode_t, struct fuse_file_info *);
  typedef int(*t_chown) (const char *, uid_t, gid_t, fuse_file_info *);
  typedef int(*t_truncate) (const char *, off_t, fuse_file_info *);
  typedef int(*t_open) (const char *, struct fuse_file_info *);
  typedef int(*t_read) (const char *, char *, size_t, off_t,
                        struct fuse_file_info *);
  typedef int(*t_write) (const char *, const char *, size_t,
                         off_t,struct fuse_file_info *);
  typedef int(*t_statfs) (const char *, struct statvfs *);
  typedef int(*t_flush) (const char *, struct fuse_file_info *);
  typedef int(*t_release) (const char *, struct fuse_file_info *);
  typedef int(*t_fsync) (const char *, int, struct fuse_file_info *);
  typedef int(*t_setxattr) (const char *, const char *, const char *,
                            size_t, int);
  typedef int(*t_getxattr) (const char *, const char *, char *, size_t);
  typedef int(*t_listxattr) (const char *, char *, size_t);
  typedef int(*t_removexattr) (const char *, const char *);
  typedef int(*t_opendir) (const char *, struct fuse_file_info *);
  typedef int(*t_readdir) (const char *, void *, fuse_fill_dir_t, off_t,
                           struct fuse_file_info *, enum fuse_readdir_flags);
  typedef int(*t_releasedir) (const char *, struct fuse_file_info *);
  typedef int(*t_fsyncdir) (const char *, int, struct fuse_file_info *);
  typedef void *(*t_init) (struct fuse_conn_info *, struct fuse_config *cfg);
  typedef void (*t_destroy) (void *);
  typedef int(*t_access) (const char *, int);
  typedef int(*t_create) (const char *, mode_t, struct fuse_file_info *);
  typedef int(*t_lock) (const char *, struct fuse_file_info *, int cmd,
                        struct flock *);
  typedef int(*t_utimens) (const char *, const struct timespec tv[2],
                            struct fuse_file_info *fi);
  typedef int(*t_bmap) (const char *, size_t blocksize, uint64_t *idx);
  typedef int(*t_ioctl) (const char *, int cmd, void *arg,
                         struct fuse_file_info *, unsigned int flags,
                         void *data);
  typedef int(*t_poll) (const char *, struct fuse_file_info *,
                        struct fuse_pollhandle *ph, unsigned *reventsp);
	typedef int(*t_write_buf) (const char *, struct fuse_bufvec *buf, off_t off,
                             struct fuse_file_info *);
  typedef int(*t_read_buf) (const char *, struct fuse_bufvec **bufp,
                            size_t size, off_t off, struct fuse_file_info *);
  typedef int (*t_flock) (const char *, struct fuse_file_info *, int op);
  typedef int (*t_fallocate) (const char *, int, off_t, off_t,
                              struct fuse_file_info *);

  template <class T> class Fuse 
  {
  public:
    Fuse()
    {
      memset (&T::operations_, 0, sizeof (struct fuse_operations));
      load_operations_();
    }

    // no copy
    Fuse(const Fuse&) = delete;
    Fuse& operator=(const Fuse&) = delete;
    Fuse& operator= (Fuse&&) = delete;

    ~Fuse() = default;

    auto run(int argc, char **argv)
    {
      return fuse_main(argc, argv, Operations(), this);
    }

    auto Operations() { return &operations_; }

    static auto this_()
    {
      return static_cast<T*>(fuse_get_context()->private_data);
    }

  private:
      
    static void load_operations_()
    {
      operations_.getattr = T::client_getattr;
      operations_.readlink = T::client_readlink;
      operations_.mknod = T::client_mknod;
      operations_.mkdir = T::client_mkdir;
      operations_.unlink = T::client_unlink;
      operations_.rmdir = T::client_rmdir;
      operations_.symlink = T::client_symlink;
      operations_.rename = T::client_rename;
      operations_.link = T::client_link;
      operations_.chmod = T::client_chmod;
      operations_.chown = T::client_chown;
      operations_.truncate = T::client_truncate;
      operations_.open = T::client_open;
      operations_.read = T::client_read;
      operations_.write = T::client_write;
      operations_.statfs = T::client_statfs;
      operations_.flush = T::client_flush;
      operations_.release = T::client_release;
      operations_.fsync = T::client_fsync;
      operations_.setxattr = T::client_setxattr;
      operations_.getxattr = T::client_getxattr;
      operations_.listxattr = T::client_listxattr;
      operations_.removexattr = T::client_removexattr;
      operations_.opendir = T::client_opendir;
      operations_.readdir = T::client_readdir;
      operations_.releasedir = T::client_releasedir;
      operations_.fsyncdir = T::client_fsyncdir;
      operations_.init = T::client_init;
      operations_.destroy = T::client_destroy;
      operations_.access = T::client_access;
      operations_.create = T::client_create;
      operations_.lock = T::client_lock;
      operations_.utimens = T::client_utimens;
      operations_.bmap = T::client_bmap;
      operations_.ioctl = T::client_ioctl;
      operations_.poll = T::client_poll;
      operations_.write_buf = T::client_write_buf;
      operations_.read_buf = T::client_read_buf;
      operations_.flock = T::client_flock;
      operations_.fallocate = T::client_fallocate;
    }

    static struct fuse_operations operations_;

    static t_getattr client_getattr;
    static t_readlink client_readlink;
    static t_mknod client_mknod;
    static t_mkdir client_mkdir;
    static t_unlink client_unlink;
    static t_rmdir client_rmdir;
    static t_symlink client_symlink;
    static t_rename client_rename;
    static t_link client_link;
    static t_chmod client_chmod;
    static t_chown client_chown;
    static t_truncate client_truncate;
    static t_open client_open;
    static t_read client_read;
    static t_write client_write;
    static t_statfs client_statfs;
    static t_flush client_flush;
    static t_release client_release;
    static t_fsync client_fsync;
    static t_setxattr client_setxattr;
    static t_getxattr client_getxattr;
    static t_listxattr client_listxattr;
    static t_removexattr client_removexattr;
    static t_opendir client_opendir;
    static t_readdir client_readdir;
    static t_releasedir client_releasedir;
    static t_fsyncdir client_fsyncdir;
    static t_init client_init;
    static t_destroy client_destroy;
    static t_access client_access;
    static t_create client_create;
    static t_lock client_lock;
    static t_utimens client_utimens;
    static t_bmap client_bmap;
    static t_ioctl client_ioctl;
    static t_poll client_poll;
    static t_write_buf client_write_buf;
    static t_read_buf client_read_buf;
    static t_flock client_flock;
    static t_fallocate client_fallocate;
  };
};

#endif
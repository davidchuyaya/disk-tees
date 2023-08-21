//==================================================================
/**
 *  Mostly copied from https://github.com/jachappell/Fusepp/tree/master.
 *  Couldn't figure out how to import with FetchContent.
 * 
 *  FuseApp -- A simple C++ wrapper for the FUSE filesystem
 *
 *  Copyright (C) 2017 by James A. Chappell (rlrrlrll@gmail.com)
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

#include "Fuse.hpp"

template<class T> Fusepp::t_getattr Fusepp::Fuse<T>::client_getattr = nullptr;
template<class T> Fusepp::t_readlink Fusepp::Fuse<T>::client_readlink = nullptr;
template<class T> Fusepp::t_mknod Fusepp::Fuse<T>::client_mknod = nullptr;
template<class T> Fusepp::t_mkdir Fusepp::Fuse<T>::client_mkdir = nullptr;
template<class T> Fusepp::t_unlink Fusepp::Fuse<T>::client_unlink = nullptr;
template<class T> Fusepp::t_rmdir Fusepp::Fuse<T>::client_rmdir = nullptr;
template<class T> Fusepp::t_symlink Fusepp::Fuse<T>::client_symlink = nullptr;
template<class T> Fusepp::t_rename Fusepp::Fuse<T>::client_rename = nullptr;
template<class T> Fusepp::t_link Fusepp::Fuse<T>::client_link = nullptr;
template<class T> Fusepp::t_chmod Fusepp::Fuse<T>::client_chmod = nullptr;
template<class T> Fusepp::t_chown Fusepp::Fuse<T>::client_chown = nullptr;
template<class T> Fusepp::t_truncate Fusepp::Fuse<T>::client_truncate = nullptr;
template<class T> Fusepp::t_open Fusepp::Fuse<T>::client_open = nullptr;
template<class T> Fusepp::t_read Fusepp::Fuse<T>::client_read = nullptr;
template<class T> Fusepp::t_write Fusepp::Fuse<T>::client_write = nullptr;
template<class T> Fusepp::t_statfs Fusepp::Fuse<T>::client_statfs = nullptr;
template<class T> Fusepp::t_flush Fusepp::Fuse<T>::client_flush = nullptr;
template<class T> Fusepp::t_release Fusepp::Fuse<T>::client_release = nullptr;
template<class T> Fusepp::t_fsync Fusepp::Fuse<T>::client_fsync = nullptr;
template<class T> Fusepp::t_setxattr Fusepp::Fuse<T>::client_setxattr = nullptr;
template<class T> Fusepp::t_getxattr Fusepp::Fuse<T>::client_getxattr = nullptr;
template<class T> Fusepp::t_listxattr Fusepp::Fuse<T>::client_listxattr = nullptr;
template<class T> Fusepp::t_removexattr Fusepp::Fuse<T>::client_removexattr = nullptr;
template<class T> Fusepp::t_opendir Fusepp::Fuse<T>::client_opendir = nullptr;
template<class T> Fusepp::t_readdir Fusepp::Fuse<T>::client_readdir = nullptr;
template<class T> Fusepp::t_releasedir Fusepp::Fuse<T>::client_releasedir = nullptr;
template<class T> Fusepp::t_fsyncdir Fusepp::Fuse<T>::client_fsyncdir = nullptr;
template<class T> Fusepp::t_init Fusepp::Fuse<T>::client_init = nullptr;
template<class T> Fusepp::t_destroy Fusepp::Fuse<T>::client_destroy = nullptr;
template<class T> Fusepp::t_access Fusepp::Fuse<T>::client_access = nullptr;
template<class T> Fusepp::t_create Fusepp::Fuse<T>::client_create = nullptr;
template<class T> Fusepp::t_lock Fusepp::Fuse<T>::client_lock = nullptr;
template<class T> Fusepp::t_utimens Fusepp::Fuse<T>::client_utimens = nullptr;
template<class T> Fusepp::t_bmap Fusepp::Fuse<T>::client_bmap = nullptr;
template<class T> Fusepp::t_poll Fusepp::Fuse<T>::client_poll = nullptr;
template<class T> Fusepp::t_write_buf Fusepp::Fuse<T>::client_write_buf = nullptr;
template<class T> Fusepp::t_read_buf Fusepp::Fuse<T>::client_read_buf = nullptr;
template<class T> Fusepp::t_flock Fusepp::Fuse<T>::client_flock = nullptr;
template<class T> Fusepp::t_fallocate Fusepp::Fuse<T>::client_fallocate = nullptr;

template<class T> struct fuse_operations Fusepp::Fuse<T>::operations_;
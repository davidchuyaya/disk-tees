set $dir=/home/davidchuyaya/disk-tees/build/client0/shim
# $dir should be set by a bash script before this file is run
set mode quit alldone
set $nfiles=1
set $meandirwidth=1
set $nthreads=1
set $io_size=4k
# Avoid out-of-memory error
set $mem_size=8k
set $file_size=64g

define file name=bigfile, path=$dir, size=$file_size, prealloc
define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file, filesetname=bigfile, iosize=$io_size, fd=1
                flowop fsync name=fsync-file, fd=1
        }
}

create files
# Drop cache, as recommended by the Filebench paper
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
# Since fsyncs are slow, just time out after 5 minutes
psrun -10 300
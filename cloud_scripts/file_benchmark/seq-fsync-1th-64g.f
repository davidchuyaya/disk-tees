# $dir should be set by a bash script before this file is run
set mode quit alldone
set $nfiles=1
set $meandirwidth=1
set $nthreads=1
set $io_size=4k
set $iterations=16777216
set $file_size=64g

define file name=bigfile, path=$dir, size=$file_size
define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=$io_size, instances=$nthreads
        {
                flowop createfile name=create1, filesetname=bigfile, fd=1
                flowop write name=write-file, filesetname=bigfile, iosize=$io_size, iters=$iterations, directio, dsync, fd=1
                flowop closefile name=close1, fd=1
                flowop finishoncount name=finish, value=1
        }
}

create files
# Drop cache, as recommended by the Filebench paper
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
psrun -10
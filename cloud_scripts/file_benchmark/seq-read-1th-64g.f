# $dir should be set by a bash script before this file is run
set mode quit alldone
set $nfiles=1
set $meandirwidth=1
set $nthreads=1
set $io_size=4k
set $iterations=16777216
set $file_size=64g

define fileset name=bigfileset, path=$dir, entries=$nfiles, dirwidth=$meandirwidth, size=$file_size, prealloc

define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open1, filesetname=bigfileset, fd=1
                flowop read name=read-file, filesetname=bigfileset, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close1, fd=1
                flowop finishoncount name=finish, value=1
        }
}

create files
# Drop cache, as recommended by the Filebench paper
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
psrun -10
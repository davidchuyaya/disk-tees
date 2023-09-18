# $dir should be set by a bash script before this file is run
set mode quit timeout
set $nfiles=32
set $meandirwidth=32
set $nthreads=1
set $io_size=4k
# Avoid out-of-memory error
set $mem_size=8k
set $file_size=2g

define fileset name=bigfileset, path=$dir, entries=$nfiles, dirwidth=$meandirwidth,dirgamma=0,size=$file_size, prealloc

define process name=filesequentialfsync, instances=1
{
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file1, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=1
                flowop fsync name=fsync-file1, fd=1, indexed=1
        }
        
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file2, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=2
                flowop fsync name=fsync-file2, fd=1, indexed=2
        }
        
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file3, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=3
                flowop fsync name=fsync-file3, fd=1, indexed=3
        }
 
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file4, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=4
                flowop fsync name=fsync-file4, fd=1, indexed=4
        }
 
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file5, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=5
                flowop fsync name=fsync-file5, fd=1, indexed=5
        }
        
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file6, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=6
                flowop fsync name=fsync-file6, fd=1, indexed=6
        }
        
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file7, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=7
                flowop fsync name=fsync-file7, fd=1, indexed=7
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file8, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=8
                flowop fsync name=fsync-file8, fd=1, indexed=8
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file9, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=9
                flowop fsync name=fsync-file9, fd=1, indexed=9
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file10, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=10
                flowop fsync name=fsync-file10, fd=1, indexed=10
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file11, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=11
                flowop fsync name=fsync-file11, fd=1, indexed=11
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file12, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=12
                flowop fsync name=fsync-file12, fd=1, indexed=12
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file13, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=13
                flowop fsync name=fsync-file13, fd=1, indexed=13
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file14, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=14
                flowop fsync name=fsync-file14, fd=1, indexed=14
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file15, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=15
                flowop fsync name=fsync-file15, fd=1, indexed=15
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file16, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=16
                flowop fsync name=fsync-file16, fd=1, indexed=16
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file17, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=17
                flowop fsync name=fsync-file17, fd=1, indexed=17
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file18, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=18
                flowop fsync name=fsync-file18, fd=1, indexed=18
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file19, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=19
                flowop fsync name=fsync-file19, fd=1, indexed=19
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file20, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=20
                flowop fsync name=fsync-file20, fd=1, indexed=20
        }
        
        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file21, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=21
                flowop fsync name=fsync-file21, fd=1, indexed=21
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file22, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=22
                flowop fsync name=fsync-file22, fd=1, indexed=22
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file23, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=23
                flowop fsync name=fsync-file23, fd=1, indexed=23
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file24, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=24
                flowop fsync name=fsync-file24, fd=1, indexed=24
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file25, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=25
                flowop fsync name=fsync-file25, fd=1, indexed=25
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file26, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=26
                flowop fsync name=fsync-file26, fd=1, indexed=26
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file27, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=27
                flowop fsync name=fsync-file27, fd=1, indexed=27
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file28, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=28
                flowop fsync name=fsync-file28, fd=1, indexed=28
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file29, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=29
                flowop fsync name=fsync-file29, fd=1, indexed=29
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file30, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=30
                flowop fsync name=fsync-file30, fd=1, indexed=30
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file31, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=31
                flowop fsync name=fsync-file31, fd=1, indexed=31
        }

        thread name=filewriter, memsize=$mem_size, instances=$nthreads
        {
                flowop write name=write-file32, filesetname=bigfileset, iosize=$io_size, fd=1, indexed=32
                flowop fsync name=fsync-file32, fd=1, indexed=32
        }
}

create files
# Drop cache, as recommended by the Filebench paper
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
# Since fsyncs are slow, just time out after 5 minutes
psrun -10 300
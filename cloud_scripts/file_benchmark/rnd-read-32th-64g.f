# $dir should be set by a bash script before this file is run
set mode quit timeout
set $nfiles=32
set $meandirwidth=32
set $nthreads=1
set $io_size=4k
set $iterations=524288
set $file_size=2g

define fileset name=bigfileset, path=$dir, entries=$nfiles, dirwidth=$meandirwidth, dirgamma=0, size=$file_size, prealloc

define process name=filereader,instances=1
{
        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open1, indexed=1, filesetname=bigfileset, fd=1
                flowop read name=read-file-1, indexed=1, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close1, indexed=1, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open2, indexed=2, filesetname=bigfileset, fd=1
                flowop read name=read-file-2, indexed=2, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close2, indexed=2, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open3, indexed=3, filesetname=bigfileset, fd=1
                flowop read name=read-file-3, indexed=3, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close3, indexed=3, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open4, indexed=4, filesetname=bigfileset, fd=1
                flowop read name=read-file-4, indexed=4, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close4, indexed=4, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open5, indexed=5, filesetname=bigfileset, fd=1
                flowop read name=read-file-5, indexed=5, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close5, indexed=5, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open6, indexed=6, filesetname=bigfileset, fd=1
                flowop read name=read-file-6, indexed=6, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close6, indexed=6, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open7, filesetname=bigfileset, fd=1, indexed=7
                flowop read name=read-file-7, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=7
                flowop closefile name=close7, fd=1, indexed=7
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open8, filesetname=bigfileset, fd=1, indexed=8
                flowop read name=read-file-8,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=8
                flowop closefile name=close8, fd=1, indexed=8
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open9, filesetname=bigfileset, fd=1, indexed=9
                flowop read name=read-file-9, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=9
                flowop closefile name=close9, fd=1, indexed=9
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open10, filesetname=bigfileset, fd=1, indexed=10
                flowop read name=read-file-10,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=10
                flowop closefile name=close10, fd=1, indexed=10
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open11, filesetname=bigfileset, fd=1, indexed=11
                flowop read name=read-file-11,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=11
                flowop closefile name=close11, fd=1, indexed=11
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open12, filesetname=bigfileset, fd=1, indexed=12
                flowop read name=read-file-12,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=12
                flowop closefile name=close12, fd=1, indexed=12
                flowop finishoncount name=finish,value=1
        }
        
        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open13, indexed=13, filesetname=bigfileset, fd=1
                flowop read name=read-file-13, indexed=13, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close13, indexed=13, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open14, indexed=14, filesetname=bigfileset, fd=1
                flowop read name=read-file-14, indexed=14, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close14, indexed=14, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open15, indexed=15, filesetname=bigfileset, fd=1
                flowop read name=read-file-15, indexed=15, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close15, indexed=15, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open16, indexed=16, filesetname=bigfileset, fd=1
                flowop read name=read-file-16, indexed=16, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close16, indexed=16, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open17, indexed=17, filesetname=bigfileset, fd=1
                flowop read name=read-file-17, indexed=17, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close17, indexed=17, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open18, indexed=18, filesetname=bigfileset, fd=1
                flowop read name=read-file-18, indexed=18, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close18, indexed=18, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open19, filesetname=bigfileset, fd=1, indexed=19
                flowop read name=read-file-19, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=19
                flowop closefile name=close19, fd=1, indexed=19
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open20, filesetname=bigfileset, fd=1, indexed=20
                flowop read name=read-file-20,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=20
                flowop closefile name=close20, fd=1, indexed=20
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open21, filesetname=bigfileset, fd=1, indexed=21
                flowop read name=read-file-21, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=21
                flowop closefile name=close21, fd=1, indexed=21
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open22, indexed=22, filesetname=bigfileset, fd=1
                flowop read name=read-file-22, indexed=22, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close22, indexed=22, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open23, indexed=23, filesetname=bigfileset, fd=1
                flowop read name=read-file-23, indexed=23, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close23, indexed=23, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open24, indexed=24, filesetname=bigfileset, fd=1
                flowop read name=read-file-24, indexed=24, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close24, indexed=24, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open25, indexed=25, filesetname=bigfileset, fd=1
                flowop read name=read-file-25, indexed=25, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close25, indexed=25, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open26, indexed=26, filesetname=bigfileset, fd=1
                flowop read name=read-file-26, indexed=26, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close26, indexed=26, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open27, indexed=27, filesetname=bigfileset, fd=1
                flowop read name=read-file-27, indexed=27, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close27, indexed=27, fd=1
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open28, indexed=28, filesetname=bigfileset, fd=1
                flowop read name=read-file-28, indexed=28, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1
                flowop closefile name=close28, indexed=28, fd=1
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open29, filesetname=bigfileset, fd=1, indexed=29
                flowop read name=read-file-29, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=29
                flowop closefile name=close29, fd=1, indexed=29
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open30, filesetname=bigfileset, fd=1, indexed=30
                flowop read name=read-file-30,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=30
                flowop closefile name=close30, fd=1, indexed=30
                flowop finishoncount name=finish,value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open31, filesetname=bigfileset, fd=1, indexed=31
                flowop read name=read-file-31, filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=31
                flowop closefile name=close31, fd=1, indexed=31
                flowop finishoncount name=finish, value=1
        }

        thread name=filereaderthread,memsize=$io_size, instances=$nthreads
        {
                flowop openfile name=open32, filesetname=bigfileset, fd=1, indexed=32
                flowop read name=read-file-32,filesetname=bigfileset, random, iosize=$io_size, iters=$iterations, fd=1, indexed=32
                flowop closefile name=close32, fd=1, indexed=32
                flowop finishoncount name=finish,value=1
        }
}

create files
# Drop cache, as recommended by the Filebench paper
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
# Since random accesses are slow, just time out after 5 minutes
psrun -10 300
# Filebench benchmarks

These benchmarks are adapted from [To FUSE or not to FUSE](https://github.com/sbu-fsl/fuse-stackfs/tree/master) ([FAST '17](https://www.usenix.org/conference/fast17/technical-sessions/presentation/vangoor)), in which they benchmarked a passthrough FUSE file system, an optimized passthrough FUSE file system (multithreading on), and EXT4.
Their evaluation contained the following [Filebench](https://github.com/filebench/filebench) configurations (known as "personalities" in Filebench), whose syntax can be found [here](https://github.com/filebench/filebench/wiki/Workload-model-language):

* `seq-rd-Nth-1f`: N threads sequentially read from a single preallocated 60GB file.
* `seq-rd-32th-32f`: 32 threads sequentially read 32 preallocated 2GB files. Each thread reads its own file.
* `rnd-rd-Nth-1f`: N threads randomly read from a single preallocated 60GB file.
* `seq-wr-1th-1f`: Single thread creates and sequentially writes a new 60GB file.
* `seq-wr-32th-32f`: 32 threads sequentially write 32 new 2GB files. Each thread writes its own file.
* `rnd-wr-Nth-1f`: N threads randomly write to a single preallocated 60GB file.
* `files-cr-Nth`: N threads create 4 million 4KB files over many directories.
* `files-rd-Nth`: N threads read from 1 million preallocated 4KB files over many directories.
* `files-del-Nth`: N threads delete 4 million of preallocated 4KB files over many directories.
* `file-server`: File-server workload emulated by Filebench. Scaled up to 200,000 files.
* `mail-server`: Mail-server workload emulated by Filebench. Scaled up to 1.5 million files.
* `web-server`: Web-server workload emulated by Filebench. Scaled up to 1.5 million files.

The questions we are interested in answering in our evaluation and the variables that we should tweak are:
1. Single-threaded vs. multi-threaded FUSE. Tweak number of clients (threads).
2. Efficiency of swapping tmpfs to disk. Tweak size of file system, sequential vs random (sequential = file system can preload).
3. Throughput/latency of reads (local), writes (broadcast), and fsyncs (broadcast + block).

Therefore, we do not need to tweak:
1. The I/O size or the size of the individual files.
2. Create vs read vs delete. (They're all asynchronous)

Therefore, we designed the following tests:

1. Single-threaded vs multi-threaded FUSE (16GB files so everything stays in memory):
* [`seq-read-1th-16g`](seq-read-1th-16g.f): 1 thread sequentially reads from a single preallocated 16GB file.
* [`seq-read-32th-16g`](seq-read-32th-16g.f): 32 threads sequentially read from 32 0.5GB files. Each thread reads its own file.
* [`seq-write-1th-16g`](seq-write-1th-16g.f): 1 thread sequentially writes to a single preallocated 16GB file.
* [`seq-write-32th-16g`](seq-write-32th-16g.f): 32 threads sequentially writes to 32 0.5GB files. Each thread writes its own file.
* [`seq-fsync-1th-16g`](seq-fsync-1th-16g.f): 1 thread sequentially writes, then fsyncs, to a single preallocated 16GB file.
* [`seq-fsync-32th-16g`](seq-fsync-32th-16g.f): 32 threads sequentially writes, then fsyncs, to 32 0.5GB files. Each thread writes its own file.

2. Efficiency of swapping tmpfs to disk (64GB files so it cannot fit in memory, always 32 threads since that's no longer the variable):
* [`seq-read-32th-64g`](seq-read-32th-64g.f): 32 threads sequentially read from 32 2GB files. Each thread reads its own file.
* [`rnd-read-32th-64g`](rnd-read-32th-64g.f): 32 threads randomly read from 32 2GB files. Each thread reads its own file.
* [`seq-write-32th-64g`](seq-write-32th-64g.f): 32 threads sequentially writes to 32 2GB files. Each thread writes its own file.
* [`rnd-write-32th-64g`](rnd-write-32th-64g.f): 32 threads randomly writes to 32 2GB files. Each thread writes its own file.
* [`seq-fsync-32th-64g`](seq-fsync-32th-64g.f): 32 threads sequentially writes, then fsyncs, to 32 2GB files. Each thread writes its own file.
* [`rnd-fsync-32th-64g`](rnd-fsync-32th-64g.f): 32 threads randomly writes, then fsyncs, to 32 2GB files. Each thread writes its own file.

3. General applications:
* [`file-server`](file-server.f): File-server workload emulated by Filebench. Scaled up to 200,000 files.
* [`mail-server`](mail-server.f): Mail-server workload emulated by Filebench. Scaled up to 1.5 million files.
* [`web-server`](web-server.f): Web-server workload emulated by Filebench. Scaled up to 1.5 million files.


set mode quit alldone
set $dir=/home/ssiva/EXT4_FS
set $nfiles=1
set $nthreads=1
#Fix I/O amount to 60 G
set $memsize=1m
set $iterations=61440

define file name=bigfileset, path=$dir, size=60g, prealloc

define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=$memsize, instances=$nthreads
        {
                flowop openfile name=open1, filesetname=bigfileset, fd=1
                flowop read name=read-file, filesetname=bigfileset, random, iosize=$memsize, iters=$iterations, fd=1
                flowop closefile name=close1, fd=1
                flowop finishoncount name=finish, value=1
        }
}

create files
#mounting and unmounting for better stable results
system "sync"
system "umount /home/ssiva/EXT4_FS/"
#Change accordingly for HDD(sdb) and SSD(sdd)
system "mount -t ext4 /dev/sdb /home/ssiva/EXT4_FS"
#warm up the cache (RAM)
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
system "dd if=/home/ssiva/EXT4_FS/bigfileset/00000001/00000001 of=/dev/null bs=4096 count=1048576 &> /dev/null"
system "echo started >> cpustats.txt"
system "echo started >> diskstats.txt"
psrun -10

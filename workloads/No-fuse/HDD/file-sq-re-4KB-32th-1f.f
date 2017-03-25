set mode quit alldone
set $dir=/home/ssiva/EXT4_FS
set $nfiles=1
set $meandirwidth=1
set $nthreads=32

define fileset name=bigfileset, path=$dir, entries=$nfiles, dirwidth=$meandirwidth, size=60g, prealloc

define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=4k, instances=$nthreads
        {
                flowop openfile name=open1, filesetname=bigfileset, fd=1
                flowop read name=read-file, filesetname=bigfileset, iosize=4k, iters=15728640, fd=1
                flowop closefile name=close1, fd=1
                flowop finishoncount name=finish, value=1
        }
}
create files
#unmount and mount for better stability results
system "sync"
system "umount /home/ssiva/EXT4_FS"
#change accordingly for HDD(sdb) and SSD(sdd)
system "mount -t ext4 /dev/sdb /home/ssiva/EXT4_FS"
system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"
system "echo started >> cpustats.txt"
system "echo started >> diskstats.txt"
psrun -10

set mode quit alldone
set $dir=/home/ssiva/COM_DIR/FUSE_EXT4_FS/
set $nthreads=1
#Fix I/O amount to 60 G
define file name=bigfile, path=$dir, size=60g
define process name=fileopen, instances=1
{
        thread name=fileopener, memsize=4k, instances=$nthreads
        {
                flowop createfile name=create1, filesetname=bigfile
                flowop write name=write-file, filesetname=bigfile, iosize=4k,iters=15728640
                flowop closefile name=close1
                flowop finishoncount name=finish, value=1
        }
}
#prealloc the file on EXT4 F/S (save the time)
system "mkdir -p /home/ssiva/COM_DIR/FUSE_EXT4_FS"
system "mkdir -p /home/ssiva/COM_DIR/EXT4_FS"

create files

#Move everything created under FUSE-EXT4 dir to EXT4 (Though nothing in this case)
system "mv /home/ssiva/COM_DIR/FUSE_EXT4_FS/* /home/ssiva/COM_DIR/EXT4_FS/"

system "sync"
system "echo 3 > /proc/sys/vm/drop_caches"

#mount FUSE FS (default) on top of EXT4
system "/home/ssiva/fuse-playground/StackFS_LowLevel/StackFS_ll -s --statsdir=/tmp/ -r /home/ssiva/COM_DIR/EXT4_FS/ /home/ssiva/COM_DIR/FUSE_EXT4_FS/ > /dev/null &"

system "echo started >> cpustats.txt"
system "echo started >> diskstats.txt"
psrun -10

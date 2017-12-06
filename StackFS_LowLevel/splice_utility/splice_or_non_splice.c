#define _GNU_SOURCE
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/xattr.h>
#include <sys/syscall.h>

#define BUFSIZE (131072)	// 128 KB

/*
If fd_in refers to a pipe, then off_in must be NULL.
If fd_in does not refer to a pipe and off_in is NULL, then bytes are read from fd_in starting from the current file offset, and the current file offset is adjusted.
If fd_in does not refer to a pipe and off_in is not NULL, then off_in must point to a buffer which specifies the starting offset from which bytes will be read from fd_in;
	in this case, the current file offset of fd_in is not changed.
Analogous statements apply for fd_out and off_out.
*/

int normal_copy(int in_fd, int out_fd) {
	char buf[BUFSIZE];
	int buf_size = BUFSIZE;
	int error = -1;
	int bytes;
	int count = 1;

	while(count <= 5000) { // Repeating the copy operation count times(we basically overwrite the file so many times)
		//printf("ATTEMPT NUMBER IN NORMAL COPY: %d\n", count);
		while((bytes = read(in_fd, buf, buf_size)) > 0 ) {
			if(write(out_fd, buf, bytes) != bytes) {
				perror("write error in normal copy:");
				printf("Write error during normal copy\n");
				goto out;
			}
		}
		// Moving the offsets back to the start, for the input and output files
		if (lseek(in_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("infile lseek in normal copy");
			printf("Error in lseek of infile in normal copy\n");
			error = -1;
			goto out;
		}
		if (lseek(out_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("outfile lseek in normal copy");
			printf("Error in lseek of outfile in normal copy\n");
			error = -1;
			goto out;
		}
		count++;
	}

	error = 0;
	out:
		return error;
}

int spliced_copy(int in_fd, int out_fd) {
	int buf_size = BUFSIZE;
	int error = -1;
	int count = 1;
	int piper[2];
	loff_t in_off = 0;
	loff_t out_off = 0;
	off_t length;
	struct stat stat_buf;
	
	if(pipe(piper) < 0) {
		perror("pipe:");
		goto out;
	}
	if(fstat(in_fd, &stat_buf) < 0) {
		perror("fstat:");
		goto out_piper;
	}
	
	error = fcntl(piper[0], F_SETPIPE_SZ, buf_size);
	if (error == -1) {
		printf("************** ERROR IN SETTING THE INPUT PIPE SIZE ****************\n");
		goto out_piper;
	}

	error = fcntl(piper[1], F_SETPIPE_SZ, buf_size);
	if (error == -1) {
		printf("************** ERROR IN SETTING THE OUTPUT PIPE SIZE ****************\n");
		goto out_piper;
	}

	while(count <= 5000) { // Repeating the copy operation count times(we basically overwrite the file so many times)
		//printf("ATTEMPT NUMBER IN SPLICED COPY: %d\n", count);

		length = stat_buf.st_size;
		buf_size = BUFSIZE;
		while (length > 0) {
			if (buf_size > length) {
				//printf("REACHED HERE IN ATTEMPT: %d\n", count);
				buf_size = length;	// Splice fails if buf-size is more than remaining data to be copied
			}

			//error = splice(in_fd, &in_off, piper[1], NULL, buf_size, SPLICE_F_MOVE | SPLICE_F_MORE);

			// Move from the input file to the pipe buffer
			error = splice(in_fd, NULL, piper[1], NULL, buf_size, SPLICE_F_MOVE | SPLICE_F_MORE);	// Simulating exact as Fuse splice read
			if (error < 0) {
				perror ("splice input error");
				printf("Error in splice input\n");
				goto out_piper;
			}
			
			//error = splice(piper[0], NULL, out_fd, &out_off, buf_size, SPLICE_F_MOVE | SPLICE_F_MORE);

			// Move from the pipe buffer to the output file
			error = splice(piper[0], NULL, out_fd, NULL, buf_size, SPLICE_F_MOVE | SPLICE_F_MORE);
			//error = splice(piper[0], NULL, out_fd, NULL, buf_size, SPLICE_F_NONBLOCK);	// Simulating exact as FUSE splice write
			if (error < 0) {
				perror ("splice output error");
				printf("Error in splice output\n");
				goto out_piper;
			}

			length -= buf_size; 
		}
		/*
		off_t in_offset = lseek( in_fd, 0, SEEK_CUR );
		off_t out_offset = lseek( out_fd, 0, SEEK_CUR );
		printf("BEFORE: INPUT OFFSET AFTER SPLICE ROUND IS : %lu\n", in_offset);
		printf("BEFORE: OUTPUT OFFSET AFTER SPLICE ROUND IS : %lu\n", out_offset);
		*/
		// Moving the offsets back to the start, for the input and output files
		if (lseek(in_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("infile lseek in spliced copy");
			printf("Error in lseek of infile in spliced copy\n");
			error = -1;
			goto out;
		}
		if (lseek(out_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("outfile lseek in spliced copy");
			printf("Error in lseek of outfile in spliced copy\n");
			error = -1;
			goto out;
		}
		/*
		in_offset = lseek( in_fd, 0, SEEK_CUR );
		out_offset = lseek( out_fd, 0, SEEK_CUR );
		printf("AFTER: INPUT OFFSET AFTER SPLICE ROUND IS : %lu\n", in_offset);
		printf("AFTER: OUTPUT OFFSET AFTER SPLICE ROUND IS : %lu\n", out_offset);
		*/
		count++;
	}

	error = 0;
	out_piper:
		close(piper[0]);
		close(piper[1]);
	out:
		return error;

}



int main(int argc, char **argv) {
	char infile[256], outfile[256];
	int in_fd = -1, out_fd = -1, error = 0;
	struct timespec start, finish;
	struct timespec wall_start, wall_finish;
	long copy_timer, copy_wall_timer;
	int option = -1;

	if (argc != 4) {
		printf("%s <infile> <outfile> <1(for normal) | 2(for splice)>\n", argv[0]);
		goto infail;
	}

	infile[0] = 0;
	outfile[0] = 0;
	strncat(infile, argv[1], sizeof(infile)-1);
	strncat(outfile, argv[2], sizeof(outfile)-1);
	option = atoi(argv[3]);	// 1 for normal copy and 2 for splice copy

	in_fd = open(infile, O_RDONLY);
	if(in_fd < 0) {
		perror("infile open error");
		error = in_fd;
		goto infail;
	}

	out_fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
	if(out_fd < 0) {
		perror("outfile open error");
		error = out_fd;
		goto outfail;
	}

	// Record the start of the CPU util time for the copy operation
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	// Record the start of the wall-clock time for the copy operation
	//clock_gettime(CLOCK_REALTIME, &wall_start);

	if (option == 1) {
		error = normal_copy(in_fd, out_fd);
	} else if (option == 2) {
		error = spliced_copy(in_fd, out_fd);
	}
	if (error < 0) {
		printf("Error copying input file [%s] to output file [%s]\n", infile, outfile);
		goto fail;
	}
	
	// Record the stop of the CPU util time for the copy operation
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);
	// Record the stop of the wall-clock time for the copy operation
	//clock_gettime(CLOCK_REALTIME, &wall_finish);

	copy_timer = (finish.tv_sec * 1000000000 + finish.tv_nsec) - (start.tv_sec * 1000000000 + start.tv_nsec);
	printf("++++++++++++++ CPU UTIL time for operation is %ld ++++++++++++++\n", copy_timer);
	//copy_wall_timer = (wall_finish.tv_sec * 1000000000 + wall_finish.tv_nsec) - (wall_start.tv_sec * 1000000000 + wall_start.tv_nsec);
	//printf("++++++++++++++ WALL-CLOCK time for operation is %ld ++++++++++++++\n", copy_wall_timer);

	fail:
		close(out_fd);
	outfail:
		close(in_fd);
	infail:
		return error;
}



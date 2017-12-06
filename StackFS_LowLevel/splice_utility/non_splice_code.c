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

#define BUFSIZE (131072)

/*
If fd_in refers to a pipe, then off_in must be NULL.
If fd_in does not refer to a pipe and off_in is NULL, then bytes are read from fd_in starting from the current file offset, and the current file offset is adjusted.
If fd_in does not refer to a pipe and off_in is not NULL, then off_in must point to a buffer which specifies the starting offset from which bytes will be read from fd_in;
	in this case, the current file offset of fd_in is not changed.
Analogous statements apply for fd_out and off_out.
*/

int normal_copy(int in_fd, int out_fd) {
	char buf[BUFSIZE];
	int error = -1;
	int bytes;
	int count = 1;

	while(count <= 5) {
		printf("ATTEMPT NUMBER : %d", count);
		while((bytes = read(in_fd, buf, sizeof(buf))) > 0 ) {
			if(write(out_fd, buf, bytes) != bytes) {
				perror("write error:");
				goto out;
			}
		}
		if (lseek(in_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("lseek");
			printf("Error in lseek of infile");
			error = -1;
			goto out;
		}
		if (lseek(out_fd, 0, SEEK_SET) == (off_t) -1) {
			perror("lseek");
			printf("Error in lseek of outfile");
			error = -1;
			goto out;
		}
		count++;
	}

	error = 0;
	out:
		return error;
}


int main(int argc, char **argv) {

	char infile[256], outfile[256];
	int in_fd = -1, out_fd = -1, error = 0;
//	int pipes pp[2];
	struct timespec start, finish;
	long splice_timer;

	infile[0] = 0;
	outfile[0] = 0;
	strncat(infile, argv[1], sizeof(infile)-1);
	strncat(outfile, argv[2], sizeof(outfile)-1);
//	error = fcntl(pp[0], F_SETPIPE_SZ, BUFSIZE);
//	if (error == -1) {
//		printf("************** ERROR IN SETTING THE INPUT PIPE SIZE ****************")
//		goto infail;
//	}

//	error = fcntl(pp[1], F_SETPIPE_SZ, BUFSIZE);
//	if (error == -1) {
//		printf("************** ERROR IN SETTING THE OUTPUT PIPE SIZE ****************")
//		goto infail;
//	}

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

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	error = normal_copy(in_fd, out_fd);
	if (error < 0) {
		printf("Error copying input file [%s] to output file [%s]\n", infile, outfile);
		goto fail;
	}
	
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);

	splice_timer = (finish.tv_sec * 1000000000 + finish.tv_nsec) - (start.tv_sec * 1000000000 + start.tv_nsec);
	printf("++++++++++++++ NON-SPLICE: Time for operation is %ld ++++++++++++++\n", splice_timer);

	fail:
		close(out_fd);
	outfail:
		close(in_fd);
	infail:
		return error;
}



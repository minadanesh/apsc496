#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	int i;
	unsigned char count = 0;
	long motorpos = 1234;
//	printf("Welcome to the QNX Momentics IDE\n");

	int fd = open ( "/dev/ser2", O_RDWR );

	for (;;) {
		//prefeix each 4byte block with a zero byte (for error detection)
		//write(fd, (unsigned char[]) {(unsigned char)0}, 1);
		for (i=0; i < 4; i++) {
			write ( fd, (unsigned char[]){(unsigned char)(motorpos >> 8*i)} , 1 );
			delay(50);
		//write ( fd, motorpos, 4 );
		}
		//printf("%08X \n",motorpos);
		motorpos++;
		//printf("\n");
		//fflush(stdout);
		//delay(20);
	}

	close (fd);

	return EXIT_SUCCESS;
}

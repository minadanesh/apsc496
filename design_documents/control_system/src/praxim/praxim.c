#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <atomic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define UART2_IRQ 73

struct sigevent event;

int encM, enc12, enc5;
int error;
int fd;
unsigned char readbuffer[8];
long motorpos = 80001;

// this is the ISR
const struct sigevent *
isr_handler (void *arg, int id)
{
	//if (tcischars(fd))
		return (&event);
	//else
	//	return NULL;
}

// this thread is dedicated to handling and managing interrupts
void *
int_thread (void *arg)
{
	int id, i;
	int invalid_block = 0;

    // enable I/O privilege
    ThreadCtl (_NTO_TCTL_IO, 0);

    // initialize the hardware, etc.

    // attach the ISR to IRQ 3
    id = InterruptAttach (UART2_IRQ, isr_handler, NULL, 0, _NTO_INTR_FLAGS_TRK_MSK);
    if ( id == -1) {
    	perror("Cannot attach IRQ");
    	error = 1;
    }

    // now service the hardware when the ISR says to
    while (1)
    {
        if(InterruptWait (0, NULL) == -1) {
        	perror("Cannot wait for interrupt");
        	error = 1;
        }
		//printf("test\n");
		if (readcond(fd, readbuffer, sizeof(readbuffer), 8, 0, 0) != -1) {
		//tcflush(fd, TCIFLUSH);
			//check indices
			for (i=0; i<8; i++) {
				//printf("%i \n", (readbuffer[i] >> 5 ));
				if ((readbuffer[i] >> 5 ) != i) {
					invalid_block = 1;
					tcflush(fd, TCIFLUSH);
					break;
				}
			}
			if (!invalid_block) {
				char byte0 = ((readbuffer[0] & 0b11111) << 3) | ((readbuffer[1] >> 2) & 0b111);
				char byte1 = ((readbuffer[1] & 0b11) << 2) | ((readbuffer[2] >> 3) & 0b11);
				char byte2 = ((readbuffer[2] & 0b111) << 5) | (readbuffer[3] & 0b11111);
				char byte3 = (readbuffer[4] >> 1) & 0b1111;
				char byte4 = ((readbuffer[4] & 0b1) << 6) | ((readbuffer[5] & 0b11111) << 2) | ((readbuffer[6] >> 2) & 0b11);
				char byte5 = ((readbuffer[6] & 0b111) << 3) | ((readbuffer[7] >> 2) & 0b111);
				enc12 = ((int)byte5 << 8) | byte4;
				encM = ((int)byte3 << 8) | byte2;
				enc5 = ((int)byte1 << 8) | byte0;
				motorpos = (long)enc12 + (long)90000;
				printf("enc12:%i encM:%i enc5:%i motorpos:%li \n", enc12, encM, enc5, motorpos);
			}
		}

    }
}


int main(int argc, char *argv[]) {
	int i;
	unsigned char index = 0;
	//unsigned char bytetosend = 0;
	unsigned char bytetosend[7];

	event.sigev_notify = SIGEV_INTR;
	SIGEV_MAKE_CRITICAL(&event);

	printf("starting...\n");

    // start up a thread that is dedicated to interrupt processing
	tcflush(fd, TCIFLUSH);
    pthread_create (NULL, NULL, int_thread, NULL);

	fd = open ( "/dev/ser2", O_RDWR );

	for (;;) {
		index = 0;
		//read(fd, readbuffer, sizeof(readbuffer));
		//printf("%s", readbuffer);

		for (i=0; i < 7; i++) {
			//bytetosend = (index++ << 5) | ((motorpos >> 5*i) & (0b11111));
			bytetosend[i] = (index++ << 5) | ((motorpos >> 5*i) & (0b11111));
			//printf("%i: %08X \n",i, (motorpos >> 5*i) & 0b11111);
			//write ( fd, (unsigned char[]){bytetosend} , 1 );
			//delay(10);
		//write ( fd, motorpos, 4 );
		}
		write ( fd, bytetosend , sizeof(bytetosend) );
		//printf("%08X \n",motorpos);
		//motorpos++;
		//printf("\n");
		//fflush(stdout);
		//delay(200);
	}

	close (fd);
	return EXIT_SUCCESS;
}

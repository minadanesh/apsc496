#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <atomic.h>

#define GPIO5_IRQ 33

#define OMAP35XX_GPIO1_BASE		    0x48310000
#define OMAP35XX_GPIO2_BASE		    0x49050000
#define OMAP35XX_GPIO3_BASE		    0x49052000
#define OMAP35XX_GPIO4_BASE		    0x49054000
#define OMAP35XX_GPIO5_BASE		    0x49056000
#define OMAP35XX_GPIO6_BASE		    0x49058000
#define OMAP35XX_GPIO_SIZE		    0x1000
#define OMAP35XX_GPIO_IRQSTATUS1    0x018
#define OMAP35XX_GPIO_IRQENABLE1    0x01c
#define OMAP35XX_GPIO_OE		    0x034
#define OMAP35XX_GPIO_DATAIN		0x038
#define OMAP35XX_GPIO_DATAOUT		0x03C
#define OMAP35XX_GPIO_SETDATAOUT	0x090
#define OMAP35XX_GPIO_CLEARDATAOUT	0x094

struct sigevent event;

uintptr_t omap3530_intc_base;
uintptr_t GPIO5;

int count;
int error;
volatile uint32_t status, value, last_value;

// this is the ISR
const struct sigevent *
isr_handler (void *arg, int id)
{
    // look at the hardware to see if it caused the interrupt
    // if not, simply return (NULL);

    // in a level-sensitive environment, clear the cause of
    // the interrupt, or at least issue InterruptMask to
    // disable the PIC from reinterrupting the kernel
	//InterruptMask(GPIO5_IRQ, id);

    // return a pointer to an event structure (preinitialized
    // by main) that contains SIGEV_INTR as its notification type.
    // This causes the InterruptWait in "int_thread" to unblock.
    status = in32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1);
    value = in32(GPIO5 + OMAP35XX_GPIO_DATAIN);
    if (status & (1 << 2))
    	return (&event);
    else
    	return NULL;
}

// this thread is dedicated to handling and managing interrupts
void *
int_thread (void *arg)
{
	int id;
    // enable I/O privilege
    ThreadCtl (_NTO_TCTL_IO, 0);

    // initialize the hardware, etc.
    omap3530_intc_base = mmap_device_io(0x1000, 0x48200000);
    GPIO5 = mmap_device_io(0x1000, OMAP35XX_GPIO5_BASE);

    // attach the ISR to IRQ 3
    id = InterruptAttach (GPIO5_IRQ, isr_handler, NULL, 0, _NTO_INTR_FLAGS_TRK_MSK);
    if ( id == -1) {
    	perror("Cannot attach IRQ");
    	error = 1;
    }
    out32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1, in32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1) | 0xFFFFFFFF);
    //out32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1, 0xFFFFFFFF);
	//out32(omap3530_intc_base + 0x48, 0x3); //NEWIRQAGR
	//out32(omap3530_intc_base + 0xa8, 0x2);
    //out32(GPIO_IRQENABLE1, in32(GPIO_IRQENABLE1)|(1<<3));
    //status = in32(GPIO_IRQSTATUS1);
    //irqenable = in32(GPIO_IRQENABLE1);
    //newirq = in32(omap3530_intc_base + 0x48);

    // perhaps boost this thread's priority here

    // now service the hardware when the ISR says to
    while (1)
    {
        if(InterruptWait (0, NULL) == -1) {
        	perror("Cannot wait for interrupt");
        	error = 1;
        }

        // at this point, when InterruptWait unblocks,
        // the ISR has returned a SIGEV_INTR, indicating
        // that some form of work needs to be done.
        status = in32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1);
        if (status & (1 << 2)) {
			//printf("count=%i status=%x value=%i last_value=%i\n", count,status,value & (1 << 3),last_value);
			//fflush(stdout);
			//if ((value & (1 << 2)) != last_value)
				count++;
			//last_value = value & (1 << 2);
			//atomic_add(&count, 1);
			// do the work

			// if the isr_handler did an InterruptMask, then
			// this thread should do an InterruptUnmask to
			// allow interrupts from the hardware
			//out32(omap3530_intc_base + 0x48, 0x3); //NEWIRQAGR
        }
    	out32(omap3530_intc_base + 0xa8, 0x2);
        out32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1, in32(GPIO5 + OMAP35XX_GPIO_IRQSTATUS1) | 0xFFFFFFFF);

        //InterruptUnmask(GPIO5_IRQ, id);
    }
   	munmap_device_io((uint64_t)0x48200048, 0x1000);
}


int main(int argc, char *argv[]) {
	event.sigev_notify = SIGEV_INTR;

	printf("Creating interrupt thread...\n");

    // perform initializations, etc.

    // start up a thread that is dedicated to interrupt processing
    pthread_create (NULL, NULL, int_thread, NULL);
    delay(10);

    while(!error) {
    	printf("count=%i status=%x value=%i last_value=%i\n", count,status,value & (1 << 2),last_value);
    	fflush(stdout);
    	sleep(2);
    }

    printf("error\n");

    // perform other processing, as appropriate

	return EXIT_SUCCESS;
}

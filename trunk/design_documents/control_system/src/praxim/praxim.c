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

#define OMAP3530_INTC_BASE			0x48200000
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

#define ENCM_A	(1 << 2)
#define ENCM_B	(1 << 3)
#define ENC12_A	(1 << 4)
#define ENC12_B	(1 << 5)
#define ENC5_A	(1 << 6)
#define ENC5_B	(1 << 7)

struct sigevent event;

uintptr_t omap_intc;
uintptr_t gpio_ptr;

int count, ENCM_count;
int error;
uint32_t irqstatus, gpio_value;

// this is the ISR
const struct sigevent *
isr_handler (void *arg, int id)
{
	//InterruptMask(GPIO5_IRQ, id);

    irqstatus = in32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1);

    if (irqstatus & (0b111111 << 2))
    	return (&event);
    else
    	return NULL;
}

// this thread is dedicated to handling and managing interrupts
void *
int_thread (void *arg)
{
	int id;
	enum encm_state {
		encm_stateA=0b10,
		encm_stateB=0b00,
		encm_stateC=0b01,
		encm_stateD=0b11} ENCM_curstate;

    // enable I/O privilege
    ThreadCtl (_NTO_TCTL_IO, 0);

    // initialize the hardware, etc.
    omap_intc = mmap_device_io(OMAP35XX_GPIO_SIZE, OMAP3530_INTC_BASE);
    gpio_ptr = mmap_device_io(OMAP35XX_GPIO_SIZE, OMAP35XX_GPIO5_BASE);

    // attach the ISR to IRQ 3
    id = InterruptAttach (GPIO5_IRQ, isr_handler, NULL, 0, _NTO_INTR_FLAGS_TRK_MSK);
    if ( id == -1) {
    	perror("Cannot attach IRQ");
    	error = 1;
    }

    //clear ENCM,12,5 interrupts
    out32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1, in32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1) | (0b111111 << 2));

    // now service the hardware when the ISR says to
    while (1)
    {
        if(InterruptWait (0, NULL) == -1) {
        	perror("Cannot wait for interrupt");
        	error = 1;
        }

        //irqstatus = in32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1);
        gpio_value = in32(gpio_ptr + OMAP35XX_GPIO_DATAIN);

        //ENCM
        if (irqstatus & (ENCM_A + ENCM_B)) {
			switch (ENCM_curstate = ((gpio_value & (ENCM_A + ENCM_B)) >> 2)) {
				case encm_stateA:
				case encm_stateC:	if (irqstatus & ENCM_A) ENCM_count++;
									else ENCM_count--;
									break;
				case encm_stateB:
				case encm_stateD:	if (irqstatus & ENCM_B) ENCM_count++;
									else ENCM_count--;
									break;
				default: 			break;
			}
        }
        //printf("curstate=%x\n",ENCM_curstate);

        //if (status & (1 << 3)) {
			//printf("count=%i status=%x value=%i last_value=%i\n", count,status,value & (1 << 3),last_value);
			//fflush(stdout);
			//if ((gpio_value & (1 << 3)) != last_value)
			//	count++;
			//last_value = gpio_value & (1 << 3);
			//atomic_add(&count, 1);
			// do the work

        //}
        out32(omap_intc + 0x48, 0x3); //NEWIRQAGR
    	out32(omap_intc + 0xa8, 0x2);
        out32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1, in32(gpio_ptr + OMAP35XX_GPIO_IRQSTATUS1) | (0b111111 << 2));

        //InterruptUnmask(GPIO5_IRQ, id);
    }
}


int main(int argc, char *argv[]) {
	event.sigev_notify = SIGEV_INTR;
	SIGEV_MAKE_CRITICAL(&event);

	printf("Creating interrupt thread...\n");

    // start up a thread that is dedicated to interrupt processing
    pthread_create (NULL, NULL, int_thread, NULL);
    delay(10);

    while(!error) {
    	printf("count=%i\n", ENCM_count);
    	//fflush(stdout);
    	sleep(2);
    }

    printf("error\n");

	return EXIT_SUCCESS;
}

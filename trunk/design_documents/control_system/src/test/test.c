#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <sys/syspage.h>
#include <errno.h>
#include <string.h>

#define CONTROL_PADCONF_MMC2_CLK 0x48002158
// Base address of GPIO1
#define OMAP35XX_GPIO1_BASE		    0x48310000
// Base address of GPIO2
#define OMAP35XX_GPIO2_BASE		    0x49050000
// Base address of GPIO3
#define OMAP35XX_GPIO3_BASE		    0x49052000
// Base address of GPIO4
#define OMAP35XX_GPIO4_BASE		    0x49054000
// Base address of GPIO5
#define OMAP35XX_GPIO5_BASE		    0x49056000
// Base address of GPIO6
#define OMAP35XX_GPIO6_BASE		    0x49058000
// GPIO register size
#define OMAP35XX_GPIO_SIZE		    0x1000
// This register is used to enable the pins output capabilities.
#define OMAP35XX_GPIO_OE		    0x034
//This register is used to register the data that is read from the GPIO pins.
#define OMAP35XX_GPIO_DATAIN		0x038
//This register is used for setting the value of the GPIO output pins
#define OMAP35XX_GPIO_DATAOUT		0x03C
//This register is used to register the data that is read from the GPIO pins.
#define OMAP35XX_GPIO_SETDATAOUT	0x090
//This register is used for setting the value of the GPIO output pins
#define OMAP35XX_GPIO_CLEARDATAOUT	0x094

uint64_t gpio_base[] = {
		OMAP35XX_GPIO1_BASE,
		OMAP35XX_GPIO2_BASE,
		OMAP35XX_GPIO3_BASE,
		OMAP35XX_GPIO4_BASE,
		OMAP35XX_GPIO5_BASE,
		OMAP35XX_GPIO6_BASE,
NULL};

uintptr_t ptr = 0;

int main(int argc, char *argv[]) {
	unsigned pad;
	int gpio_module = 5;
	int i=0;

	/* enable this thread to execute i/o functions... */
   	ThreadCtl(_NTO_TCTL_IO, 0);
   	/* map get an address to io from os */
   	//ptr = mmap_device_memory( 0, OMAP35XX_GPIO_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, gpio_base[gpio_module - 1]);
   	ptr = mmap_device_io(OMAP35XX_GPIO_SIZE, gpio_base[gpio_module - 1]);
   	if ( ptr == MAP_FAILED ) {
   	    perror( "mmap_device_memory for physical address failed");
   	    exit( EXIT_FAILURE );
   	}

	printf("Welcome to the QNX Momentics IDE\n");

	//for (i; i<10; i++) {
	while (1) {
		//printf("on\n");
		out32(ptr + OMAP35XX_GPIO_DATAOUT, in32(ptr + OMAP35XX_GPIO_DATAOUT) | (1 << 4));
		delay(3000);
		//printf("off\n");
		out32(ptr + OMAP35XX_GPIO_DATAOUT, in32(ptr + OMAP35XX_GPIO_DATAOUT) & ~(1 << 4));
		delay(3000);
	}

   	//printf("pad=%x\n",pad);
   	printf("done\n");

   	munmap_device_io(gpio_base[gpio_module - 1], OMAP35XX_GPIO_SIZE);
   	//munmap_device_memory(gpio_base[gpio_module - 1], OMAP35XX_GPIO_SIZE);
	return EXIT_SUCCESS;
}

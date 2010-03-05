/*
 * This program enables the user to fiddle with GPIO pins.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

// See page 3460 of SPRUF98D–October 2009
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

#define CONTROL_PADCONF_MMC2_CLK 0x48002158

uint64_t gpio_base[] = {
		OMAP35XX_GPIO1_BASE,
		OMAP35XX_GPIO2_BASE,
		OMAP35XX_GPIO3_BASE,
		OMAP35XX_GPIO4_BASE,
		OMAP35XX_GPIO5_BASE,
		OMAP35XX_GPIO6_BASE,
NULL};

uintptr_t ptr = 0;
uintptr_t ctrl_ptr = 0;


void gpio_set(int pin)
{
	//out32(ptr + OMAP35XX_GPIO1_CLEARDATAOUT,  (1 << pin));
	out32(ptr + OMAP35XX_GPIO_DATAOUT, in32(ptr + OMAP35XX_GPIO_DATAOUT) | (1 << pin));
}

void gpio_reset(int pin)
{
	//out32(ptr + OMAP35XX_GPIO1_SETDATAOUT,  (1 << pin));
	out32(ptr + OMAP35XX_GPIO_DATAOUT, in32(ptr + OMAP35XX_GPIO_DATAOUT) & ~(1 << pin));
}

int gpio_read(int direction, int pin)
{
	int pin_value = 0;
	if (direction == 0){
		if (in32(ptr + OMAP35XX_GPIO_DATAOUT) & (1 << pin)){
			pin_value = 1;
		}
	}
	else if (direction == 1){
		if (in32(ptr + OMAP35XX_GPIO_DATAIN) & (1 << pin)){
			pin_value = 1;
		}
	}
	else{
		pin_value = -1;
	}
	return pin_value;
}

void gpio_set_direction(int direction, int pin)
{
	if (direction == 0){
		out32(ptr + OMAP35XX_GPIO_OE, in32(ptr + OMAP35XX_GPIO_OE) & ~(1 << pin));
		//out32(ptr + OMAP35XX_GPIO_OE, 0x00000000);
	} else {
		out32(ptr + OMAP35XX_GPIO_OE, in32(ptr + OMAP35XX_GPIO_OE) | (1 << pin));
		//out32(ptr + OMAP35XX_GPIO_OE, 0xffffffff );
	}
}

int main (int argc, char *argv[])
{
	int opt = 0, verbose = 0, pin = -1, direction = -1, gpio_module = -1;
	int cmd = -1;
	extern char *optarg;

	// Handle commandline arguments
	// -m gpio_module  The GPIO module. Can be a value from 1 to 6
	// -p pin          The GPIO pin you want to set
	// -d direction    The direction of the GPIO pin. Can be: 0 (=write)|1(=read)
	// -c command      Action to perform. Can be: set|reset|read
	while ((opt = getopt(argc, argv, "m:p:d:c:v")) != -1) {
		switch (opt) {
		case 'm':
			gpio_module = strtol(optarg, NULL, 10);
			if (errno != 0) gpio_module = -1;
			break;
		case 'p':
			pin = strtol(optarg, NULL, 10);
			if (errno != 0) pin = -1;
			break;
		case 'd':
			direction = strtol(optarg, NULL, 10);
			if (errno != 0 || direction > 1) direction = -1;

			if      (strcmp(optarg, "write") == 0) direction = 0;
			else if (strcmp(optarg, "read")  == 0) direction = 1;
			else direction = -1;
		case 'c':
			if      (strcmp(optarg, "set")   == 0) cmd = 0;
			else if (strcmp(optarg, "reset") == 0) cmd = 1;
			else if (strcmp(optarg, "read")  == 0) cmd = 2;
			else cmd = -1;
			break;
		case 'v':
			verbose++;
			break;
		default:
			break;
		}
	}

	if (gpio_module != -1 && pin != -1 && direction != -1 && cmd != -1) {

		/* enable this thread to execute i/o functions... */
	   	ThreadCtl(_NTO_TCTL_IO, 0);

	   	//ptr = mmap_device_memory( 0, OMAP35XX_GPIO_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, gpio_base[gpio_module - 1]);
	   	ptr = mmap_device_io(OMAP35XX_GPIO_SIZE, gpio_base[gpio_module - 1]);
	   	ctrl_ptr = mmap_device_io(OMAP35XX_GPIO_SIZE, CONTROL_PADCONF_MMC2_CLK);
	   	if ( (void*)ptr == MAP_FAILED ) {
	   	    perror( "mmap_device_memory for physical address failed");
	   	    exit( EXIT_FAILURE );
	   	}

	   	printf("ctrl_ptr %x\n",in32(ctrl_ptr));
	   	//out32(ctrl_ptr, 0x01040000);

	   	gpio_set_direction(pin, direction);

	   	if (cmd == 0) gpio_set(pin);
	   	if (cmd == 1) gpio_reset(pin);
	   	if (cmd == 2) printf("0x%x\n",gpio_read(direction, pin));

	   	//munmap_device_memory(gpio_base[gpio_module - 1], OMAP35XX_GPIO_SIZE);
	   	munmap_device_io(gpio_base[gpio_module - 1], OMAP35XX_GPIO_SIZE);
	   	munmap_device_io(CONTROL_PADCONF_MMC2_CLK, OMAP35XX_GPIO_SIZE);
		return 0;
	} else {
		printf("Illigal commandline options provided, type 'use %s' on the commandline for usage information:\n", argv[0]);
		return -1;
	}
}

/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $ 
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/slog.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <fs/etfs.h>
#include <unistd.h>
#include <arm/omap2420.h>
#include <arm/omap3530.h>



 
struct chipio;
#define CHIPIO	struct chipio
#include "./devio.h"



/* NAND Address split up macros */
#define NAND_ADDR_COL1(addr)    ((addr) & 0xff)					// Column address CA[7:0]
#define NAND_ADDR_COL2(addr)    (((addr) & 0xf00) >>8)			// Column address CA[11:8], CA[11]=1, then CA[10:6] must be "0" 
#define NAND_ADDR_ROW1(page)    ((page) & 0xff)					// Page address PA[5:0] and Block address BA[7:6]
#define NAND_ADDR_ROW2(page)    (((page) & 0xff00) >> 8)		// Block address BA[15:8]
#define NAND_ADDR_ROW3(page)    (((page) & 0x70000) >> 16)		// Block address BA[18:16]

/* 
 * GPMC NAND Access Macros 
 * OMAP3530 has a General-Purpose Memory Controller (GPMC)
 * The NAND on the Mistral and Beagle is on chip select 0.
 * The GPMC includes command, address and data registers for accessing the NAND.
 * 
 * NAND devices require multiple address programming phases. The  software driver is responsible for
 * issuing the correct number of command and address program accesses, according to the device
 * command set and the device address-mapping scheme.
 *  
 */

/* 
 *GPMC Macros 
 */
/* The configuration register allows global configuration of the GPMC */
#define OMAP_GPMC_CONFIG                 ((volatile unsigned int *  )(cio->gpmc_vbase+0x50))

/* The status register provides global status bits of the GPMC */
#define OMAP_READ_GPMC_STATUS()        (*((volatile unsigned int *  )(cio->gpmc_vbase+0x54)))


/* 
 * GPMC Nand/Chip select Macros 
 */
/* GPMC_NAND_COMMAND_0 This register is not a true register, just an address location. Used to send commands to Nand*/
#define OMAP_WRITE_NAND_COMMAND(cmd)   (*((volatile unsigned short *)(cio->gpmc_nand_vbase+0x7C)) = cmd)
/* 
 * GPMC_NAND_ADDRESS_0 This register is not a true register, just an address location.  
 * Used to send address to Nand (multiple cycles) 
 */
#define OMAP_WRITE_NAND_ADDRESS(addr)  (*((volatile unsigned short *)(cio->gpmc_nand_vbase+0x80)) = addr)

/* GPMC_NAND_DATA_0 This register is not a true register, just an address location.
 * Write/read data from/to Nand 
 */
#define OMAP_WRITE_NAND_DATA(data)     (*((volatile unsigned short *)(cio->gpmc_nand_vbase+0x84)) = data)
#define OMAP_READ_NAND_DATA()          (*((volatile unsigned short *)(cio->gpmc_nand_vbase+0x84)))


#define OMAP_MAX_WAIT_PINS    4
#define OMAP_MAX_CHIP_SELECT  7
#define OMAP_MAX_BUSY_LOOP   10
#define OMAP_GPMC_WAITPIN0    0x100


/*
 * Device specific data structure for the OMAP3530, with 2048 byte Micron Nand.
 * 
 * Mistral NAND ID on Chip JY192 => MT29C2G24MAKLAJG-6 IT ES
 * Beagle  NAND ID on Chip JW192 => MT29C2G24MAKLAJG-6 IT
 * 
 * 2Gb NAND/Mobile DDR SDRAM : MT29C2G24MAKLAJG-6 IT
 * MT29C 2G 24M A K LAJG-6
 * MT29C  - Combined part NAND LPDRAM 
 * 2G       - NAND 2G density
 * 24M      -  1024Mb LPDRAM
 * K        - x16 (nand, second gen)
 * L        - x32 
 */
 
struct chipio {
	struct _chipio	chip;
	uintptr_t       gpmc_pbase;
	uintptr_t		gpmc_vbase;
	uintptr_t		gpmc_nand_vbase;
	uint8_t			cs;  /* GPMC chip select */
	uint8_t			wp;  /* GPMC Wait pin  */
	uint8_t			cfg;
} chipio;




int
main(int argc, char *argv[]) {

	return(etfs_main(argc, argv));
}


/*
 * Process device specific options (if any).
 * This is always called before any access to the part.
 * It is called by the -D option to the filesystem. If no -D option is given
 * this function will still be called with "" for optstr.
 */

int devio_options(struct etfs_devio *dev, char *optstr) {
	struct chipio	*cio;
	char			*value;
	static char		*opts[] ={
						"use",		// 0
						"gpmc",		// 1
						"cs",		// 2
						"wp",		// 3
						"cfg",		// 4
						NULL
					} ;

	cio = dev->cio = &chipio;

	cio->gpmc_pbase = OMAP3530_GPMC_BASE;
	cio->cs         = 0;	// Chip select
	cio->wp         = 0;    // Wait pin
	cio->cfg		= 0;  //do we need to touch the OMAP_GPMC_CONFIG reg.

	while (*optstr) {
		switch (getsubopt(&optstr, opts, &value)) {
			case 0:
				fprintf(stderr, "Device specific options:\n");
				fprintf(stderr, "  -D use,gpmc=xxxx,cs=[chip select 0-7],wp=[wait pin0-3],cfg\n");
				return (-1);

			case 1:
				cio->gpmc_pbase = strtol(value, NULL, 0);
				break;

			case 2:
				cio->cs         = strtol(value, NULL, 0);
				if (cio->cs > 0 && cio->cs >= OMAP_MAX_CHIP_SELECT) {
					fprintf(stderr, "Chip select out of range 0-7\n");
					return (EINVAL);
				}
				break;
				
			case 3:
				cio->wp 		= strtol(value, NULL, 10);
				if (cio->wp > 0 && cio->wp >= OMAP_MAX_WAIT_PINS) {
					fprintf(stderr, "Wait pin out of range 0-3\n");
					return (EINVAL);
				}
				break;
			case 4:
				cio->cfg = 1;
				break;
			default:
				dev->log(_SLOG_ERROR, "Invalid -D suboption.");
				return (EINVAL);
		}
	}

	return (EOK);
}



// Called once at startup
int nand_init(struct etfs_devio *dev) {
	struct chipio	*cio = dev->cio;
	uintptr_t		base;
	uint32_t		cfg7; 

	/* Map in the device registers */
	base = cio->gpmc_vbase = mmap_device_io(OMAP3530_GPMC_SIZE, cio->gpmc_pbase);
	if (base == (uintptr_t) MAP_FAILED)
		dev->log(_SLOG_CRITICAL, "Unable to map in device registers (%d).", errno); 
		
	cio->gpmc_nand_vbase = base + (cio->cs * 0x30);
	cfg7 = in32(cio->gpmc_nand_vbase + OMAP2420_GPMC_CS0+OMAP2420_GPMC_CONFIG7);
	if (!(cfg7 & (1 << 6))) {
		dev->log(_SLOG_CRITICAL, "Chip select %d is not enabled (%d).", cio->cs, errno);
		return -1;
	}

	if(cio->cfg==0){
		/* 
		 * (1<<4) WRITEPROTECT is high
		 * (0<<0) NANDFORCEPOSTEDWRITE is off
		 */
		*OMAP_GPMC_CONFIG = (1<<4) | (0<<0);
	}

	return (0);
}


int nand_wait_busy(struct chipio *cio, uint32_t usec) {
	/* 
	 * Wait for NAND 
	 * Wait pins start at bit 8 of GPMC status register
	 * WAITxSTATUS Is a copy of input pin WAITx. (Reset value is WAITx input R xpin sampled at IC reset)
	 * 0x0: WAITx asserted (inactive state)
	 * 0x1: WAITx de-asserted
	 */
	while (usec--){
		if(OMAP_READ_GPMC_STATUS() & (OMAP_GPMC_WAITPIN0<<cio->wp))
			return 0;
		else
			nanospin_ns(1000);
	}
	return (-1);
}


// Accessing the command reg automatically sets ALE=0, CLE=1
void nand_write_cmd(struct chipio *cio, int command) {
	OMAP_WRITE_NAND_COMMAND(command);
}


// Accessing the address reg automatically sets ALE=1, CLE=0
void nand_write_pageaddr(struct chipio *cio, unsigned page, int addr_cycles, int spare) {
	int		i;
	unsigned short	addr[5];
	
	if (cio->chip.page_2048) {
		if(spare){
			//assume it's 16 bit width
			addr[0] = 0x80; 
			addr[1] = DATASIZE_2048>>9 ;
		}else{
			addr[0] = addr[1] = 0x0 ;
		}
		addr[2] = NAND_ADDR_ROW1(page) & 0xff;
		addr[3] = NAND_ADDR_ROW2(page) & 0xff;
		addr[4] = NAND_ADDR_ROW3(page) & 0xff;
	} else {
		addr[0] = 0;
		addr[1] = page;
		addr[2] = page >> 8;
		addr[3] = page >> 16;
	}

	for (i = 0; i < addr_cycles; i++)
	{
		OMAP_WRITE_NAND_ADDRESS(addr[i]);
	}
		
}


// Accessing the address reg automatically sets ALE=1, CLE=0
void nand_write_blkaddr(struct chipio *cio, unsigned blk, int addr_cycles) {
    int			i;
	unsigned short		addr[5];
	unsigned	page = blk * PAGES2BLK_2048;
	//fprintf(stderr, "%s blk %d cycles %d\n",__FUNCTION__, blk, addr_cycles); 

	if (cio->chip.page_2048) {
		addr_cycles = 3;
		addr[0] = NAND_ADDR_ROW1(page) & 0xff;
		addr[1] = NAND_ADDR_ROW2(page) & 0xff;
		addr[2] = NAND_ADDR_ROW3(page) & 0xff;
	} else {
		addr[0] = blk << 5;
		addr[1] = blk >> 3;
		addr[2] = blk >> 11;
		addr[3] = 0;
	}
	
	for (i = 0; i < addr_cycles; i++) {
		OMAP_WRITE_NAND_ADDRESS(addr[i]);
	}

}


// Accessing the data reg automatically sets ALE=0, CLE=0
void nand_write_data(struct chipio *cio, uint8_t *databuffer, int data_cycles) {
	int i;
	
	uint16_t *p1 = (uint16_t *)databuffer;

	for (i = 0; i < data_cycles; i += 2)
		OMAP_WRITE_NAND_DATA(*p1++);

}

// Accessing the data reg automatically sets ALE=0,CLE=0
void nand_read_data(struct chipio *cio, uint8_t *databuffer, int data_cycles) {
	int	i;

	uint16_t	*p1 = (uint16_t *)databuffer;

	for (i = 0; i < data_cycles; i += 2)
		*p1++ = OMAP_READ_NAND_DATA(); 
}


// Accessing the data reg automatically sets ALE=0,CLE=0
void nand_read_status(struct chipio *cio, uint8_t *databuffer, int data_cycles) {
	int i; 

	for (i = 0; i < data_cycles; i++)
		databuffer[i]= OMAP_READ_NAND_DATA();
}

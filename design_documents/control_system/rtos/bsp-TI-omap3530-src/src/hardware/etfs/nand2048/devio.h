/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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


#include <stdint.h>

//
// The spare area used for the NAND. It is 64 bytes in size.
//
struct spare {
	uint8_t		status;				// Factory marking for bad blks (0xff == GOOD)
	uint8_t		status2;			// For 16 bit wide parts
	uint8_t		align[6];
	uint32_t	erasesig[2];		// The erase signature created by devio_eraseblk

	// This must start on a 16 byte boundry because you can only write to a 16
	// byte region once for the new multilevel NAND parts.
	uint8_t		unused[6];
	uint8_t		nclusters;			// Number of clusters
	uint8_t		clusterhi;			// High order 8 bits of cluster
	uint32_t	sequence;			// Sequence number
	uint16_t	fid;				// File id
	uint16_t	clusterlo;			// Low order 16 bits of logical cluster
	uint8_t		eccv[8][3];			// ECCs for page data
	uint32_t	crcdata;			// Crc for readcluster() 
	uint32_t	crctrans;			// Crc for readtrans().
};
#define ERASESIG1	0x756c7769
#define ERASESIG2	0x70626d66


// This struct is included in the chipio structure defined in the low level board driver
struct _chipio {
	unsigned	addrcycles;
	unsigned	lastcmd;
	unsigned	lastpage;
	unsigned	inspare;
 };

#ifndef CHIPIO
#define CHIPIO struct _chipio
#endif

//
// Nand device specific data structures
//
#define NANDCMD_SPAREREAD		0x50
#define NANDCMD_READ			0x00
#define NANDCMD_READCONFIRM     0x30
#define NANDCMD_PROGRAM			0x80
#define NANDCMD_PROGRAMCONFIRM	0x10
#define NANDCMD_ERASE			0x60
#define NANDCMD_ERASECONFIRM	0xD0
#define NANDCMD_IDREAD			0x90
#define NANDCMD_STATUSREAD		0x70
#define NANDCMD_RESET			0xFF

#define DATASIZE		2048
#define SPARESIZE		64
#define PAGESIZE	(DATASIZE + SPARESIZE)
#define PAGES2BLK		64


// These timeouts are very generous.
#define MAX_RESET_USEC	600		// 600us
#define MAX_READ_USEC	50      //  50us
#define MAX_POST_USEC	2000    //   2ms
#define MAX_ERASE_USEC	10000	//  10ms


// Prototypes for chip interface
int  nand_init(struct etfs_devio *dev);
int  nand_wait_busy(CHIPIO *cio, uint32_t usec);
void nand_write_pageaddr(CHIPIO *cio, unsigned page, unsigned page_offset, int addr_cycles);
void nand_write_blkaddr(CHIPIO *cio, unsigned blk, int addr_cycles);
void nand_write_cmd(CHIPIO *cio, int command);
void nand_write_data(CHIPIO *cio, uint8_t *databuffer, int data_cycles);
void nand_read_data(CHIPIO *cio, uint8_t *databuffer, int data_cycles);

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/etfs/nand2048/devio.h $ $Rev: 249398 $" )

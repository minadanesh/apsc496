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



#ifndef __NAND_H__
#define __NAND_H__
#include "ipl.h"
#include <hw/inout.h>
#include <stdint.h>

#define	delay(x) { unsigned _delay = x; while (_delay--); }


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
#define OMAP_GPMC_CONFIG                 ((volatile unsigned int *  )(dev->iobase+0x50))
#define OMAP_GPMC_ECC_CONFIG			((volatile unsigned int *  )(dev->iobase+0x1F4))
#define OMAP_GPMC_ECC_CONTROL			((volatile unsigned int *  )(dev->iobase+0x1F8))
#define OMAP_GPMC_ECC_SIZE_CONFIG		((volatile unsigned int *  )(dev->iobase+0x1FC))
#define GPMC_ECC_RESULT(x)				(*((volatile unsigned int *  )(dev->iobase+0x200+(x)*4)))
#define GPMC_PREFETCH_CONFIG1			(*((volatile unsigned int *  )(dev->iobase+0x1e0)))
#define GPMC_PREFETCH_CONFIG2			(*((volatile unsigned int *  )(dev->iobase+0x1e4)))
#define GPMC_PREFETCH_CONTROL			(*((volatile unsigned int *  )(dev->iobase+0x1ec)))
#define GPMC_PREFETCH_STATUS			(*((volatile unsigned int *  )(dev->iobase+0x1f0)))

#define ECC_P1_128_E(val)    ((val)  & 0x000000FF)	/* Bit 0 to 7 */
#define ECC_P512_2048_E(val) (((val) & 0x00000F00)>>8)	/* Bit 8 to 11 */
#define ECC_P1_128_O(val)    (((val) & 0x00FF0000)>>16)	/* Bit 16 to Bit 23 */
#define ECC_P512_2048_O(val) (((val) & 0x0F000000)>>24)	/* Bit 24 to Bit 27 */

/* The status register provides global status bits of the GPMC */
#define OMAP_READ_GPMC_STATUS()        (*((volatile unsigned int *  )(dev->iobase+0x54)))


/* 
 * GPMC Nand/Chip select Macros 
 */
/* GPMC_NAND_COMMAND_0 This register is not a true register, just an address location. Used to send commands to Nand*/
#define OMAP_WRITE_NAND_COMMAND(cmd)   (*((volatile unsigned short *)(dev->iobase+0x7C)) = cmd)
/* 
 * GPMC_NAND_ADDRESS_0 This register is not a true register, just an address location.  
 * Used to send address to Nand (multiple cycles) 
 */
#define OMAP_WRITE_NAND_ADDRESS(addr)  (*((volatile unsigned short *)(dev->iobase+0x80)) = addr)

/* GPMC_NAND_DATA_0 This register is not a true register, just an address location.
 * Write/read data from/to Nand 
 */
#define OMAP_WRITE_NAND_DATA(data)     (*((volatile unsigned short *)(dev->iobase+0x84)) = data)
#define OMAP_READ_NAND_DATA()          (*((volatile unsigned short *)(dev->iobase+0x84)))



#define OMAP_MAX_WAIT_PINS    4
#define OMAP_MAX_CHIP_SELECT  7
#define OMAP_MAX_BUSY_LOOP   10
#define OMAP_GPMC_WAITPIN0    0x100

#define	OMAP3_DMA4_BASE			0x48056000
#define	DMA4_IRQSTATUS(j)		(0x08 + (j) * 4)	// j = 0 - 3
#define	DMA4_IRQENABLE(j)		(0x18 + (j) * 4)	// j = 0 - 3
#define	DMA4_SYSSTATUS			(0x28)
#define	DMA4_OCP_SYSCONFIG		(0x2C)
#define	DMA4_CAPS_0				(0x64)
#define	DMA4_CAPS_2				(0x6C)
#define	DMA4_CAPS_3				(0x70)
#define	DMA4_CAPS_4				(0x74)
#define	DMA4_GCR				(0x78)
#define	DMA4_CCR(i)				(0x80 + (i) * 0x60)	// i = 0 - 31
#define	DMA4_CLNK_CTRL(i)		(0x84 + (i) * 0x60)
#define	DMA4_CICR(i)			(0x88 + (i) * 0x60)
#define	DMA4_CSR(i)				(0x8C + (i) * 0x60)
#define	DMA4_CSDP(i)			(0x90 + (i) * 0x60)
#define	DMA4_CEN(i)				(0x94 + (i) * 0x60)
#define	DMA4_CFN(i)				(0x98 + (i) * 0x60)
#define	DMA4_CSSA(i)			(0x9C + (i) * 0x60)
#define	DMA4_CDSA(i)			(0xA0 + (i) * 0x60)
#define	DMA4_CSE(i)				(0xA4 + (i) * 0x60)
#define	DMA4_CSF(i)				(0xA8 + (i) * 0x60)
#define	DMA4_CDE(i)				(0xAC + (i) * 0x60)
#define	DMA4_CDF(i)				(0xB0 + (i) * 0x60)
#define	DMA4_CSAC(i)			(0xB4 + (i) * 0x60)
#define	DMA4_CDAC(i)			(0xB8 + (i) * 0x60)
#define	DMA4_CCEN(i)			(0xBC + (i) * 0x60)
#define	DMA4_CCFN(i)			(0xC0 + (i) * 0x60)
#define	DMA4_COLOR(i)			(0xC4 + (i) * 0x60)

#define	DMA4_GPMC_DMA			3

#define	DMA4_CCR_SYNCHRO_CONTROL(s)		(((s) & 0x1F) | (((s) >> 5) << 19))
#define	OMAP3_SDMA_ERROR	((1 << 11) | (1 << 10) | (1 << 9) | (1 << 8))

typedef struct {
	volatile uint32_t	ccr;		// 0x00
	volatile uint32_t	clnk_ctrl;	// 0x04
	volatile uint32_t	cicr;		// 0x08
	volatile uint32_t	csr;		// 0x0C
	volatile uint32_t	csdp;		// 0x10
	volatile uint32_t	cen;		// 0x14
	volatile uint32_t	cfn;		// 0x18
	volatile uint32_t	cssa;		// 0x1C
	volatile uint32_t	cdsa;		// 0x20
	volatile uint32_t	cse;		// 0x24
	volatile uint32_t	csf;		// 0x28
	volatile uint32_t	cde;		// 0x2C
	volatile uint32_t	cdf;		// 0x30
	volatile uint32_t	csac;		// 0x34
	volatile uint32_t	cdac;		// 0x38
	volatile uint32_t	ccen;		// 0x3C
	volatile uint32_t	ccfn;		// 0x40
	volatile uint32_t	color;		// 0x44
} dma4_param;

//
// Nand device specific data structures
// 2K page size parts
//
#define NANDCMD_SPAREREAD_2048			0x50
#define NANDCMD_READ_2048				0x00
#define NANDCMD_READCONFIRM_2048     	0x30
#define NANDCMD_PROGRAM_2048			0x80
#define NANDCMD_PROGRAMCONFIRM_2048		0x10
#define NANDCMD_ERASE_2048				0x60
#define NANDCMD_ERASECONFIRM_2048		0xD0
#define NANDCMD_IDREAD_2048				0x90
#define NANDCMD_STATUSREAD_2048			0x70
#define NANDCMD_RESET_2048				0xFF
#define NANDCMD_READ_SEQUENTIAL_2048   	0x31
#define NANDCMD_READ_LAST_2048   		0x3F
#define NANDCMD_UNLOCK_LOW				0x23
#define NANDCMD_UNLOCK_HIGH				0x24


#define DATASIZE_2048					2048
#define SPARESIZE_2048					64
#define PAGESIZE_2048					(DATASIZE_2048 + SPARESIZE_2048)
#define PAGES2BLK_2048					64
#define PAGE_MASK						0x3F




// These timeouts are very generous.
#define MAX_RESET_USEC				200*600		// 600us
#define MAX_READ_USEC				200*150      //  50us
#define MAX_POST_USEC				200*2000    //   2ms
#define MAX_ERASE_USEC				200*10000	//  10ms

#define NAND_BLK_VALID				0xFFFF

typedef struct nand_chip {
	unsigned		iobase;
	unsigned 		numblks;
	unsigned 		addrcycles;
	unsigned 		sparesize; //     = SPARESIZE * PAGES2CLUSTER;
	unsigned		blksize; //       = (dev->clustersize + SPARESIZE) * dev->clusters2blk;
} NAND_CHIP;

NAND_CHIP chip_dev;
NAND_CHIP *dev;

typedef struct _PageInfo
{
	uint16_t	status;				// Factory set - FFFF means good. 
	uint8_t		ecc[12];
	uint8_t		reserv0[2];
	uint32_t	sequence;
	uint8_t		reserv1[44];
}PageInfo, *PPageInfo;

unsigned nand_init();
unsigned nand_wait_busy(NAND_CHIP *dev, uint32_t usec);
int nand_read_page(NAND_CHIP *dev, unsigned page, uint8_t *databuffer, int data_cycles, int op, uint8_t *ecc);
int nand_write_page(NAND_CHIP *dev, unsigned page, uint8_t *databuffer, uint8_t *sparebuffer);
int nand_erase_blk(NAND_CHIP *dev, unsigned blk);
unsigned GetBlockStatus(NAND_CHIP *dev, unsigned blockID);
unsigned read_image_from_nand(unsigned start_block, unsigned end_block, unsigned dst);
unsigned upgrade_nand_flash(unsigned start_block, unsigned end_block, unsigned src);
unsigned upgrade_IPL(unsigned src);
unsigned nand_unlock_blocks(unsigned start, unsigned end);

#endif /*__NAND_H__*/


__SRCVERSION( "$URL$ $Rev$" );

/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems. 
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


#ifndef	_SDHCI_INCLUDED
#define	_SDHCI_INCLUDED


#define	__REG8(x)		(*((volatile uint8_t  *)(x)))
#define	__REG16(x)		(*((volatile uint16_t *)(x)))
#define	__REG32(x)		(*((volatile uint32_t *)(x)))

/*
 * SDIO/MMC Memory-mapped registers
 */
#define	SDHCI_DMAADR(base)		__REG32(base + 0x00)
#define	SDHCI_BLKSZ(base)		__REG16(base + 0x04)
#define	SDHCI_BLKCNT(base)		__REG16(base + 0x06)
#define	SDHCI_CMDARG(base)		__REG32(base + 0x08)
#define	SDHCI_XFRMODE(base)		__REG16(base + 0x0C)
#define	SDHCI_SDCMD(base)		__REG16(base + 0x0E)
#define	SDHCI_RESP0(base)		__REG32(base + 0x10)
#define	SDHCI_RESP1(base)		__REG32(base + 0x14)
#define	SDHCI_RESP2(base)		__REG32(base + 0x18)
#define	SDHCI_RESP3(base)		__REG32(base + 0x1C)
#define	SDHCI_BUFDATA(base)		__REG32(base + 0x20)
#define	SDHCI_PSTATE(base)		__REG32(base + 0x24)
#define	SDHCI_HOSTCTL(base)		__REG8 (base + 0x28)
#define	SDHCI_PWRCTL(base)		__REG8 (base + 0x29)
#define	SDHCI_BLKGAPCTL(base)	__REG8 (base + 0x2A)
#define	SDHCI_WAKECTL(base)		__REG8 (base + 0x2B)
#define	SDHCI_CLKCTL(base)		__REG16(base + 0x2C)
#define	SDHCI_TOCTL(base)		__REG8 (base + 0x2E)
#define	SDHCI_SWRST(base)		__REG8 (base + 0x2F)
#define	SDHCI_NINTSTS(base)		__REG16(base + 0x30)
#define	SDHCI_ERINTSTS(base)	__REG16(base + 0x32)
#define	SDHCI_NINTEN(base)		__REG16(base + 0x34)
#define	SDHCI_ERINTEN(base)		__REG16(base + 0x36)
#define	SDHCI_NINTSIGEN(base)	__REG16(base + 0x38)
#define	SDHCI_ERINTSIGEN(base)	__REG16(base + 0x3A)
#define	SDHCI_AC12ERRSTS(base)	__REG16(base + 0x3C)
#define	SDHCI_CAP(base)			__REG32(base + 0x40)
#define	SDHCI_MCCAP(base)		__REG32(base + 0x48)
#define	SDHCI_SLTINTSTS(base)	__REG16(base + 0xFC)
#define	SDHCI_CTRLRVER(base)	__REG16(base + 0xFE)


/*
 * Transfer Mode Register (XFRMODE) bit defination
 */
#define	SDHCI_XFRMODE_DMAEN		(1 << 0)	// DMA enable
#define	SDHCI_XFRMODE_BCE		(1 << 1)	// Block Count Enable
#define	SDHCI_XFRMODE_AC12EN	(1 << 2)	// Auto CMD12 Enable
#define	SDHCI_XFRMODE_DATDIR	(1 << 4)	// Data Direction
#define	SDHCI_XFRMODE_MBS		(1 << 5)	// Multiple Block Select

/*
 * Command Register (CMD) bit defination
 */
#define	SDHCI_CMD_RSPLEN0		(0 << 0)	// No response
#define	SDHCI_CMD_RSPLEN136		(1 << 0)	// 136 bit response
#define	SDHCI_CMD_RSPLEN48		(2 << 0)	// 48 bit response
#define	SDHCI_CMD_RSPLEN48b		(3 << 0)	// 48 bit response with busy bit check
#define	SDHCI_CMD_CCCE			(1 << 3)	// Command CRC Check enable
#define	SDHCI_CMD_CICE			(1 << 4)	// Command Index Check enable
#define	SDHCI_CMD_DPS			(1 << 5)	// Data Present
#define	SDHCI_CMD_NORMAL		(0 << 6)	// Normal Command
#define	SDHCI_CMD_CMDIDX(x)		(((x) & 0x3F) << 8)


/*
 * Present State Register (PSTATE) bit defination
 */
#define	SDHCI_PSTATE_WP			(1 << 19)
#define	SDHCI_PSTATE_CD			(1 << 18)
#define	SDHCI_PSTATE_CSS		(1 << 17)
#define	SDHCI_PSTATE_CI			(1 << 16)
#define	SDHCI_PSTATE_BRE		(1 << 11)
#define	SDHCI_PSTATE_BUFWREN	(1 << 10)
#define	SDHCI_PSTATE_RTA		(1 <<  9)
#define	SDHCI_PSTATE_WTA		(1 <<  8)
#define	SDHCI_PSTATE_DLA		(1 <<  2)
#define	SDHCI_PSTATE_DCI		(1 <<  1)
#define	SDHCI_PSTATE_CINH		(1 <<  0)

/*
 * Normal Interrupt status/enable/signal enable bit defination
 */
#define	SDHCI_NINT_CC			(1 << 0)	// Command Complete
#define	SDHCI_NINT_TC			(1 << 1)	// Transfer Complete
#define	SDHCI_NINT_BGE			(1 << 2)	// Block Gap Event
#define	SDHCI_NINT_DMA			(1 << 3)	// DMA Interrupt
#define	SDHCI_NINT_BWR			(1 << 4)	// Buffer Write Ready
#define	SDHCI_NINT_BRR			(1 << 5)	// Buffer Read Ready
#define	SDHCI_NINT_CIN			(1 << 6)	// Card Insertion
#define	SDHCI_NINT_CRM			(1 << 7)	// Card Removal
#define	SDHCI_NINT_CI			(1 << 8)	// Card Interrupt
#define	SDHCI_NINT_EI			(1 << 15)	// Error Interrupt, for status register only

/*
 * Error Interrupt status/enable/signal enable bit defination
 */
#define	SDHCI_ERINT_CTE			(1 << 0)	// Command Timeout Error
#define	SDHCI_ERINT_CCE			(1 << 1)	// Command CRC Error
#define	SDHCI_ERINT_CEBE		(1 << 2)	// Command End Bit Error
#define	SDHCI_ERINT_CIE			(1 << 3)	// Command Index Error
#define	SDHCI_ERINT_DTE			(1 << 4)	// Data Timeout Error
#define	SDHCI_ERINT_DCE			(1 << 5)	// Data CRC Error
#define	SDHCI_ERINT_DEBE		(1 << 6)	// Data End Bit Error
#define	SDHCI_ERINT_CL			(1 << 7)	// Current Limit Error
#define	SDHCI_ERINT_AC12		(1 << 8)	// Auto CMD12 Error

/*
 * Clock Control Register bit defination
 */
#define	SDHCI_CLKCTL_ICE		(1 << 0)	// Internal Clock Enable
#define	SDHCI_CLKCTL_ICS		(1 << 1)	// Internal Clock Stable
#define	SDHCI_CLKCTL_CLKEN		(1 << 2)	// Clock Enable

/*
 * Host Control Register bit defination
 */
#define	SDHCI_HOSTCTL_LEDCTL	(1 << 0)	// LED Control
#define	SDHCI_HOSTCTL_DTW1BIT	(0 << 1)	// Data Bus Width 1 bit
#define	SDHCI_HOSTCTL_DTW4BIT	(1 << 1)	// Data Bus Width 4 bit
#define	SDHCI_HOSTCTL_HSEN		(1 << 2)	// High Speed Enable
#define	SDHCI_HOSTCTL_MMC8              (1 << 3)        // Data Bus Width 8 bit (MMC)

/*
 * Power Control Register bit defination
 */
#define	SDHCI_PWRCTL_PWREN		(1 << 0)	// SD Bus Power Enable
#define	SDHCI_PWRCTL_V33		(7 << 1)	// 3.3V

/*
 * Software Reset Register bit defination
 */
#define	SDHCI_SWRST_ALL			(1 << 0)	// Reset All
#define	SDHCI_SWRST_RC			(1 << 1)	// Reset CMD
#define	SDHCI_SWRST_RD			(1 << 2)	// Reset Data

/*
 * Capability Register bit defination
 */
#define	SDHCI_CAP_S18			(1 << 26)	// 1.8V support
#define	SDHCI_CAP_S30			(1 << 25)	// 3.0V support
#define	SDHCI_CAP_S33			(1 << 24)	// 3.3V support
#define	SDHCI_CAP_SRS			(1 << 23)	// Suspend/Resume support
#define	SDHCI_CAP_DMA			(1 << 22)	// DMA support
#define	SDHCI_CAP_HS			(1 << 21)	// High-Speed support
#define	SDHCI_CAP_MBL512		(0 << 16)	// Max block length 512
#define	SDHCI_CAP_MBL2048		(2 << 16)	// Max block length 2048


typedef	struct _sdhci_ext {
	int			pci_hdl;
	void		*pci_dev_hdl;
	uintptr_t	base;
	uint8_t 	*tbuf;
	paddr_t		tbuf_phy;
	int 		pio_enable;
	int			blksz;
	uint32_t	clock;
	void		*hba;
} sdhci_ext_t;

extern int sdhci_init(SIM_HBA *hba);

#endif

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devb/mmcsd/chipsets/sim_sdhci.h $ $Rev: 249398 $" )

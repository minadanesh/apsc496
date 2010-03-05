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




/*
 * i.MX51 processor
 *
 */

#ifndef	__ARM_MX51_H_INCLUDED
#define	__ARM_MX51_H_INCLUDED


/*
 * Enhanced Secured Digital Host Controllers
 */
#define	MX51_ESDHC1_BASE		0x70004000
#define	MX51_ESDHC2_BASE		0x70008000
#define	MX51_ESDHC3_BASE		0x70020000
#define	MX51_ESDHC4_BASE		0x70024000
#define	MX51_ESDHC_SIZE			0x4000

/*
 * UARTs
 */
#define	MX51_UART3_BASE			0x700C0000


/*
 * Enhanced Configurable Serial Peripheral Interfaces
 */
#define	MX51_ECSPI1_BASE		0x70010000
#define	MX51_ECSPI_SIZE			0x4000

/*
 * Synchronous Serial Interfaces
 */
#define	MX51_SSI2_BASE			0x70014000
#define	MX51_SSI_SIZE			0x4000

/*
 * Sony/Philips Digital Interface Transmitter
 */
#define	MX51_SPDIF_BASE			0x70028000
#define	MX51_SPDIF_SIZE			0x4000

/*
 * High Speed Inter IC
 */
#define	MX51_HS_I2C_BASE		0x70038000
#define	MX51_HS_I2C_SIZE		0x4000

/*
 * GPIOs
 */
#define	MX51_GPIO1_BASE			0x73F84000
#define	MX51_GPIO2_BASE			0x73F88000
#define	MX51_GPIO3_BASE			0x73F8C000
#define	MX51_GPIO4_BASE			0x73F90000
#define	MX51_GPIO_SIZE			0x4000

/*
 * Keypad port
 */
#define	MX51_KPP_BASE			0x73F94000
#define	MX51_KPP_SIZE			0x4000

/*
 * Watchdogs
 */
#define	MX51_WDOG1_BASE			0x73F98000
#define	MX51_WDOG2_BASE			0x73F9C000
#define	MX51_WDOG_SIZE			0x4000

/*
 * General Purpose Timer
 */
#define	MX51_GPT_BASE			0x73FA0000
#define	MX51_GPT_SIZE			0x4000

/*
 * Secure Real Time Clock
 */
#define	MX51_SRTC_BASE			0x73FA4000
#define	MX51_SRTC_SIZE			0x4000

/*
 * I/O MUX Controller
 */
#define	MX51_IOMUXC_BASE		0x73FA8000
#define	MX51_IOMUXC_SIZE		0x4000

/* IOMUX Registers, offset from base address */
#define	MX51_IOMUX_GPR0			0x000
#define	MX51_IOMUX_GPR1			0x004

#define	MX51_IOMUX_SWMUX		0x00
#define	MX51_IOMUX_SWPAD		0x3F0
#define MX51_IOMUX_SWINPUT		0x8C4

/*
 * Enhanced Periodic Timers
 */
#define	MX51_EPIT1_BASE			0x73FAC000
#define	MX51_EPIT2_BASE			0x73FB0000
#define	MX51_EPIT_SIZE			0x4000



#endif	/* __ARM_MX51_H_INCLUDED */

/* __SRCVERSION("mx51.h $Rev: 169789 $"); */

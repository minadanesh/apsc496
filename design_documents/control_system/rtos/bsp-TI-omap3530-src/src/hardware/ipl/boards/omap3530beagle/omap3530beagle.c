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


#include "ipl.h"
#include <arm/omap2420.h>
#include <arm/omap3530.h>
#include <hw/inout.h>
#include "nand.h"
#include "beagle_mux.h"
#include "omap3530beagle.h"


#define LDELAY      	12000000
#define PLL_STOP        1	/* PER & IVA */
#define PLL_LOW_POWER_BYPASS   5	/* MPU, IVA & CORE */
#define PLL_FAST_RELOCK_BYPASS 6	/* CORE */
#define PLL_LOCK        7	/* MPU, IVA, CORE & PER */

/* CORE DPLL */
#define CORE_M3X2      2	/* 332MHz : CM_CLKSEL1_EMU */
#define CORE_SSI_DIV   3	/* 221MHz : CM_CLKSEL_CORE */
#define CORE_FUSB_DIV  2	/* 41.5MHz: */
#define CORE_L4_DIV    2	/*  83MHz : L4 */
#define CORE_L3_DIV    2	/* 166MHz : L3 {DDR} */
#define GFX_DIV        2	/*  83MHz : CM_CLKSEL_GFX */
#define WKUP_RSM       2	/* 41.5MHz: CM_CLKSEL_WKUP */

/* PER DPLL */
#define PER_M6X2       3	/* 288MHz: CM_CLKSEL1_EMU */
#define PER_M5X2       4	/* 216MHz: CM_CLKSEL_CAM */
#define PER_M4X2       9	/* 96MHz : CM_CLKSEL_DSS-dss1 */
#define PER_M3X2       16	/* 54MHz : CM_CLKSEL_DSS-tv */

#define CLSEL1_EMU_VAL ((CORE_M3X2 << 16) | (PER_M6X2 << 24) | (0x0a50))

#define GPMC_SYSCONFIG		(0x10)
#define GPMC_IRQSTATUS		(0x18)
#define GPMC_IRQENABLE		(0x1C)
#define GPMC_TIMEOUT_CONTROL	(0x40)
#define GPMC_CONFIG			(0x50)
#define GPMC_STATUS			(0x54)

#define GPMC_CONFIG1		(0x60)
#define GPMC_CONFIG2		(0x64)
#define GPMC_CONFIG3		(0x68)
#define GPMC_CONFIG4		(0x6C)
#define GPMC_CONFIG5		(0x70)
#define GPMC_CONFIG6		(0x74)
#define GPMC_CONFIG7		(0x78)

/* SDRC */
#define SDRC_SYSCONFIG		0x10
#define SDRC_STATUS		0x14
#define SDRC_CS_CFG		0x40
#define SDRC_SHARING		0x44
#define SDRC_DLLA_CTRL		0x60
#define SDRC_DLLA_STATUS	0x64
#define SDRC_DLLB_CTRL		0x68
#define SDRC_DLLB_STATUS	0x6C

#define SMX_APE_BASE			0x68000000
#define PM_RT_APE_BASE_ADDR_ARM		(SMX_APE_BASE + 0x10000)
#define PM_GPMC_BASE_ADDR_ARM		(SMX_APE_BASE + 0x12400)
#define PM_OCM_RAM_BASE_ADDR_ARM	(SMX_APE_BASE + 0x12800)
#define PM_OCM_ROM_BASE_ADDR_ARM	(SMX_APE_BASE + 0x12C00)
#define PM_IVA2_BASE_ADDR_ARM		(SMX_APE_BASE + 0x14000)

#define RT_REQ_INFO_PERMISSION_1	(PM_RT_APE_BASE_ADDR_ARM + 0x68)
#define RT_READ_PERMISSION_0		(PM_RT_APE_BASE_ADDR_ARM + 0x50)
#define RT_WRITE_PERMISSION_0		(PM_RT_APE_BASE_ADDR_ARM + 0x58)
#define RT_ADDR_MATCH_1			(PM_RT_APE_BASE_ADDR_ARM + 0x60)

#define GPMC_REQ_INFO_PERMISSION_0	(PM_GPMC_BASE_ADDR_ARM + 0x48)
#define GPMC_READ_PERMISSION_0		(PM_GPMC_BASE_ADDR_ARM + 0x50)
#define GPMC_WRITE_PERMISSION_0		(PM_GPMC_BASE_ADDR_ARM + 0x58)

#define OCM_REQ_INFO_PERMISSION_0	(PM_OCM_RAM_BASE_ADDR_ARM + 0x48)
#define OCM_READ_PERMISSION_0		(PM_OCM_RAM_BASE_ADDR_ARM + 0x50)
#define OCM_WRITE_PERMISSION_0		(PM_OCM_RAM_BASE_ADDR_ARM + 0x58)
#define OCM_ADDR_MATCH_2		(PM_OCM_RAM_BASE_ADDR_ARM + 0x80)

#define IVA2_REQ_INFO_PERMISSION_0	(PM_IVA2_BASE_ADDR_ARM + 0x48)
#define IVA2_READ_PERMISSION_0		(PM_IVA2_BASE_ADDR_ARM + 0x50)
#define IVA2_WRITE_PERMISSION_0		(PM_IVA2_BASE_ADDR_ARM + 0x58)

#define IVA2_REQ_INFO_PERMISSION_1	(PM_IVA2_BASE_ADDR_ARM + 0x68)
#define IVA2_READ_PERMISSION_1		(PM_IVA2_BASE_ADDR_ARM + 0x70)
#define IVA2_WRITE_PERMISSION_1		(PM_IVA2_BASE_ADDR_ARM + 0x78)

#define IVA2_REQ_INFO_PERMISSION_2	(PM_IVA2_BASE_ADDR_ARM + 0x88)
#define IVA2_READ_PERMISSION_2		(PM_IVA2_BASE_ADDR_ARM + 0x90)
#define IVA2_WRITE_PERMISSION_2		(PM_IVA2_BASE_ADDR_ARM + 0x98)

#define IVA2_REQ_INFO_PERMISSION_3	(PM_IVA2_BASE_ADDR_ARM + 0xA8)
#define IVA2_READ_PERMISSION_3		(PM_IVA2_BASE_ADDR_ARM + 0xB0)
#define IVA2_WRITE_PERMISSION_3		(PM_IVA2_BASE_ADDR_ARM + 0xB8)

#define SMS_BASE                    0x6c000000
#define SMS_RG_ATT_0             SMS_BASE + 0x48

#define OMAP35XX_CM_CLKEN2_PLL		0x48004D04
#define OMAP35XX_CM_IDLEST2_CKGEN	0x48004D24
#define OMAP35XX_CM_CLKSEL4_PLL		0x48004D4C
#define OMAP35XX_CM_CLKSEL5_PLL		0x48004D50

#define CONTROL_PBIAS_LITE			0x48002520
#define CONTROL_DEVCONF0 			0x48002274
#define CONTROL_DEVCONF1 			0x480022D8
/* DEVCONF0 Bits */
#define MMCSDIO1ADPCLKISEL 			(1<<24)
#define MCBSP2_CLKS 				(1<<6)
#define MCBSP1_CLKS 				(1<<2)
#define MCBSP1_FSR 					(1<<4)	/* Receive FS sourced from Transmit FS */
#define MCBSP1_CLKR 				(1<<3)	/* Receive CLK sourced from Transmit CLK */
/* DEVCONF1 Bits */
#define MCBSP3_CLKS 				(1<<0)

#define OMAP35XX_GPIO6_OE				0x49058034
#define OMAP35XX_GPIO6_DATAOUT	0x4905803C
#define OMAP35XX_GPIO6		    		160


extern void v7_flush_dcache_all(int type);

int boot_from_flash=0;

/*****************************************
 * Routine: secure_unlock
 * Description: Setup security registers for access 
 * (GP Device only)
 *****************************************/
static void secure_unlock(void)
{
	/* Permission values for registers -Full fledged permissions to all */
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	/* Protection Module Register Target APE (PM_RT)*/
	out32(RT_REQ_INFO_PERMISSION_1, UNLOCK_1);
	out32(RT_READ_PERMISSION_0, UNLOCK_1);
	out32(RT_WRITE_PERMISSION_0, UNLOCK_1);
	out32(RT_ADDR_MATCH_1, UNLOCK_2);

	out32(GPMC_REQ_INFO_PERMISSION_0, UNLOCK_3);
	out32(GPMC_READ_PERMISSION_0, UNLOCK_3);
	out32(GPMC_WRITE_PERMISSION_0, UNLOCK_3);

	out32(OCM_REQ_INFO_PERMISSION_0, UNLOCK_3);
	out32(OCM_READ_PERMISSION_0, UNLOCK_3);
	out32(OCM_WRITE_PERMISSION_0, UNLOCK_3);
	out32(OCM_ADDR_MATCH_2, UNLOCK_2);

	/* IVA Changes */
	out32(IVA2_REQ_INFO_PERMISSION_0, UNLOCK_3);
	out32(IVA2_READ_PERMISSION_0, UNLOCK_3); 
	out32(IVA2_WRITE_PERMISSION_0, UNLOCK_3); 

	out32(SMS_RG_ATT_0, UNLOCK_1); /* SDRC region 0 public */
}

static inline unsigned get_device_type(void)
{
	return ( (in32(OMAP3530_SYSCTL_BASE+OMAP3530_SYSCTL_STATUS)>>8) & 0x7 );
}

/**********************************************************
 * Routine: setup_auxcr()
 * Description: Write to AuxCR desired value using SMI.
 *  general use.
 ***********************************************************/
static inline void setup_auxcr()
{
	unsigned long i;
	volatile unsigned int j;
	/* Save r0, r12 and restore them after usage */
	__asm__ __volatile__("mov %0, r12":"=r" (j));
	__asm__ __volatile__("mov %0, r0":"=r" (i));

	/* GP Device ROM code API usage here */
	/* r12 = AUXCR Write function and r0 value */
	__asm__ __volatile__("mov r12, #0x3");
	__asm__ __volatile__("mrc p15, 0, r0, c1, c0, 1");
	/* Enabling ASA */
	__asm__ __volatile__("orr r0, r0, #0x10");
	/* SMI instruction to call ROM Code API */
	__asm__ __volatile__(".word 0xE1600070");
	__asm__ __volatile__("mov r0, %0":"=r" (i));
	__asm__ __volatile__("mov r12, %0":"=r" (j));
}

/*****************************************************************
 * sr32 - clear & set a value in a bit range for a 32 bit address
 *****************************************************************/
static inline void sr32(unsigned addr, unsigned start_bit, unsigned num_bits, unsigned value)
{
	unsigned tmp, msk = 0;
	msk = 1 << num_bits;
	--msk;
	tmp = in32(addr) & ~(msk << start_bit);
	tmp |= value << start_bit;
	out32(addr, tmp);
}

static void init_clocks(void)
{
	int count = LDELAY;
	
	/* set input crystal speed --26MH*/
	out32(OMAP35XX_PRM_CLKSEL, (in32(OMAP35XX_PRM_CLKSEL) & ~0x7)|3);

	/* If the input clock is greater than 19.2M always divide/2 */
	out32(OMAP35XX_PRM_CLKSRC_CTRL, (in32(OMAP35XX_PRM_CLKSRC_CTRL) & ~0xc0) | 0x80);

	/* Bypass mode */
	out32(OMAP35XX_PRM_CLKSRC_CTRL, (in32(OMAP35XX_PRM_CLKSRC_CTRL) & ~0x3));

	/* Unlock MPU DPLL (slows things down, and needed later) */
	out32(OMAP35XX_CM_CLKEN_PLL_MPU, (in32(OMAP35XX_CM_CLKEN_PLL_MPU) & ~0x7) | PLL_LOW_POWER_BYPASS);
	while((in32(OMAP35XX_CM_IDLEST_PLL_MPU) & 1) && count--) 
		;

	/* Core DPLL targets for L3 at 166 & L133 --m: 0x14c, n: 0x0c, fsel: 0x03, m2: 0x1*/
	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0x7)) | (PLL_FAST_RELOCK_BYPASS));
	count = LDELAY;
	while((in32(OMAP35XX_CM_IDLEST_CKGEN) & 1) && count--) 
		;

	/* For OMAP3 ES1.0 Errata 1.50, default value directly doesnt
	   work. write another value and then default value. */
	out32(OMAP35XX_CM_CLKSEL1_EMU, (in32(OMAP35XX_CM_CLKSEL1_EMU) & ~(0x1f<<16)) |((CORE_M3X2 + 1)<<16));/* m3x2 */
	out32(OMAP35XX_CM_CLKSEL1_EMU, (in32(OMAP35XX_CM_CLKSEL1_EMU) & ~(0x1f<<16)) |((CORE_M3X2)<<16));/* m3x2 */

	out32(OMAP35XX_CM_CLKSEL1_PLL, (in32(OMAP35XX_CM_CLKSEL1_PLL) & ~(0x3<<27)) |(0x1<<27));/* Set M2 */
	out32(OMAP35XX_CM_CLKSEL1_PLL, (in32(OMAP35XX_CM_CLKSEL1_PLL) & ~(0x7ff<<16)) |(0x14c<<16));/* Set M */
	out32(OMAP35XX_CM_CLKSEL1_PLL, (in32(OMAP35XX_CM_CLKSEL1_PLL) & ~(0x7f<<8)) |(0x0c<<8));/* Set N */
	out32(OMAP35XX_CM_CLKSEL1_PLL, (in32(OMAP35XX_CM_CLKSEL1_PLL) & ~(0x1<<6)));/* 96M Src */
	out32(OMAP35XX_CM_CLKSEL_CORE, (in32(OMAP35XX_CM_CLKSEL_CORE) & ~(0xf<<8)) |(CORE_SSI_DIV<<8));/* ssi */
	out32(OMAP35XX_CM_CLKSEL_CORE, (in32(OMAP35XX_CM_CLKSEL_CORE) & ~(0x3<<4)) |(CORE_FUSB_DIV<<4));/* fsusb */
	out32(OMAP35XX_CM_CLKSEL_CORE, (in32(OMAP35XX_CM_CLKSEL_CORE) & ~(0x3<<2)) |(CORE_L4_DIV<<2));/* l4 */
	out32(OMAP35XX_CM_CLKSEL_CORE, (in32(OMAP35XX_CM_CLKSEL_CORE) & ~(0x3<<0)) |(CORE_L3_DIV<<0));/* l3 */
	out32(OMAP35XX_CM_CLKSEL_GFX, (in32(OMAP35XX_CM_CLKSEL_GFX) & ~(0x7<<0)) |(GFX_DIV<<0));/* gfx */
	out32(OMAP35XX_CM_CLKSEL_WKUP, (in32(OMAP35XX_CM_CLKSEL_WKUP) & ~(0x3<<1)) |(WKUP_RSM<<1));/* reset mgr */
	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0xf<<4)) |(0x3<<4));/* FREQSEL */

	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0x7)) | (PLL_LOCK));/* lock mode */

	count = LDELAY;
	while((!(in32(OMAP35XX_CM_IDLEST_CKGEN) & 1)) && count--) 
		;

	/* PER DPLL values ----m: 0x1b0, n: 0x0c, fsel: 0x3, m2: 0x9 */
	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0x7<<16)) | (PLL_STOP<<16));
	count = LDELAY;
	while((in32(OMAP35XX_CM_IDLEST_CKGEN) & 2) && count--) 
		;
	/* Errata 1.50 Workaround for OMAP3 ES1.0 only */
	/* If using default divisors, write default divisor + 1  and then the actual divisor value */
	/* Need to change it to silicon and revisino check */
	out32(OMAP35XX_CM_CLKSEL1_EMU, (in32(OMAP35XX_CM_CLKSEL1_EMU) & ~(0x1f<<24))|((PER_M6X2 + 1)<<24));/* set M6 */
	out32(OMAP35XX_CM_CLKSEL1_EMU, (in32(OMAP35XX_CM_CLKSEL1_EMU) & ~(0x1f<<24))|((PER_M6X2)<<24));/* set M6 */
	out32(OMAP35XX_CM_CLKSEL_CAM, (in32(OMAP35XX_CM_CLKSEL_CAM) & ~0x1f) | (PER_M5X2 + 1)); /* set M5 */
	out32(OMAP35XX_CM_CLKSEL_CAM, (in32(OMAP35XX_CM_CLKSEL_CAM) & ~0x1f) | (PER_M5X2 )); /* set M5 */
	out32(OMAP35XX_CM_CLKSEL_DSS, 0x00001006); /*set divide by 6, uboot using divide by 9*/
	out32(OMAP35XX_CM_CLKSEL3_PLL, (in32(OMAP35XX_CM_CLKSEL3_PLL) & ~(0x1f))|(0x9 + 1));/* set M2 */
	out32(OMAP35XX_CM_CLKSEL3_PLL, (in32(OMAP35XX_CM_CLKSEL3_PLL) & ~(0x1f))|(0x9));/* set M2 */
	out32(OMAP35XX_CM_CLKSEL2_PLL, (in32(OMAP35XX_CM_CLKSEL2_PLL) & ~(0x3ff<<8))|(0x1b0<<8)); /* set m */
	out32(OMAP35XX_CM_CLKSEL2_PLL, (in32(OMAP35XX_CM_CLKSEL2_PLL) & ~(0x7f))|(0x0c)); /* set n */
	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0xf<<20))|(0x3<<20)); /* FREQSEL */
	out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0x7<<16)) | (PLL_LOCK<<16));/* lock mode */

	count = LDELAY;
	while((!(in32(OMAP35XX_CM_IDLEST_CKGEN) & 2)) && count--) 
		;

	/* PER2 DPLL values 120M*/
	out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0x7)) | (PLL_STOP));
	count = LDELAY;
	while((in32(OMAP35XX_CM_IDLEST2_CKGEN) & 1) && count--) 
		;
	out32(OMAP35XX_CM_CLKSEL4_PLL, (120<<8)|(12));/* set M2 */
	out32(OMAP35XX_CM_CLKSEL5_PLL, 1);/* set M2 */
	out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0xf<<4))|(7<<4)); /* FREQSEL */
	out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0x7)) | (PLL_LOCK));/* lock mode */
	count = LDELAY;
	while((!(in32(OMAP35XX_CM_IDLEST2_CKGEN) & 1)) && count--) 
		;

	/* MPU DPLL (unlocked already) ---- m: 0x1F4, n: 0x0C, fsel: 0x03, m2: 0x01 -- MPU 500MHz*/
	out32(OMAP35XX_CM_CLKSEL2_PLL_MPU, (in32(OMAP35XX_CM_CLKSEL2_PLL_MPU) & ~0x1f) | 0x1);
	out32(OMAP35XX_CM_CLKSEL1_PLL_MPU, (in32(OMAP35XX_CM_CLKSEL1_PLL_MPU) & ~0x7ff00) | (0x1f4<<8));
	out32(OMAP35XX_CM_CLKSEL1_PLL_MPU, (in32(OMAP35XX_CM_CLKSEL1_PLL_MPU) & ~0x7f) | 0x0c);
	out32(OMAP35XX_CM_CLKEN_PLL_MPU, (in32(OMAP35XX_CM_CLKEN_PLL_MPU) & ~0xf0) | 0x3<<4);	/* FREQSEL */
	out32(OMAP35XX_CM_CLKEN_PLL_MPU, (in32(OMAP35XX_CM_CLKEN_PLL_MPU) & ~0x7) | PLL_LOCK);
	count = LDELAY;
	while((!(in32(OMAP35XX_CM_IDLEST_PLL_MPU) & 1)) && count--) 
		;

	/* IVA2 ---- m: 0x168, n: 0x0C, fsel: 0x03, m2: 0x01 --DSP 360MHz*/
	out32(OMAP35XX_CM_CLKEN_PLL_IVA2, (in32(OMAP35XX_CM_CLKEN_PLL_IVA2) & ~0x7)| PLL_STOP);
	count = LDELAY;
	while((in32(OMAP35XX_CM_IDLEST_PLL_IVA2) & 1) && count--) 
		;
	out32(OMAP35XX_CM_CLKSEL2_PLL_IVA2, (in32(OMAP35XX_CM_CLKSEL2_PLL_IVA2) & ~0x1f)| 0x1);/* set M2 */
	out32(OMAP35XX_CM_CLKSEL1_PLL_IVA2, (in32(OMAP35XX_CM_CLKSEL1_PLL_IVA2) &  ~0x7ff00) | (0x168<<8)); /* set M */
	out32(OMAP35XX_CM_CLKSEL1_PLL_IVA2, (in32(OMAP35XX_CM_CLKSEL1_PLL_IVA2) & ~0x7f) | 0x0c); /* set N */
	out32(OMAP35XX_CM_CLKEN_PLL_IVA2,  (in32(OMAP35XX_CM_CLKEN_PLL_IVA2) & ~0xf0) | 0x3<<4);	/* FREQSEL */
	out32(OMAP35XX_CM_CLKEN_PLL_IVA2, (in32(OMAP35XX_CM_CLKEN_PLL_IVA2) & ~0x7)| PLL_LOCK);
	count = LDELAY;
	while((!(in32(OMAP35XX_CM_IDLEST_PLL_IVA2) & 1)) && count--) 
		;

	/* Set up GPTimers to sys_clk source only */
	sr32(OMAP35XX_CM_CLKSEL_PER, 0, 8, 0xff);
	sr32(OMAP35XX_CM_CLKSEL_WKUP, 0, 1, 1);
	
	/* Enable UART1 clocks */
	sr32(OMAP35XX_CM_FCLKEN1_CORE, 13, 1, 0x1);
	sr32(OMAP35XX_CM_ICLKEN1_CORE, 13, 1, 0x1);
	
	/* UART 3 Clocks */
	sr32(OMAP35XX_CM_FCLKEN_PER, 11, 1, 0x1);
	sr32(OMAP35XX_CM_ICLKEN_PER, 11, 1, 0x1);

	/*GPIO Clocks */
	out32(OMAP35XX_CM_FCLKEN_PER, OMAP3530_FCK_PER_ON);
	out32(OMAP35XX_CM_ICLKEN_PER, OMAP3530_ICK_PER_ON);
}

static void init_watchdog(void)
{
	/* There are 3 watch dogs WD1=Secure, WD2=MPU, WD3=IVA. WD1 is 
	 * either taken care of by ROM (HS/EMU) or not accessible (GP).
	 * We need to take care of WD2-MPU or take a PRCM reset.  WD3
	 * should not be running and does not generate a PRCM reset.
	 */
	out32(OMAP35XX_CM_FCLKEN_WKUP, in32(OMAP35XX_CM_FCLKEN_WKUP) | (0x1<<5));
	out32(OMAP35XX_CM_ICLKEN_WKUP, in32(OMAP35XX_CM_ICLKEN_WKUP) | (0x1<<5));

	/*
	 * Disable Watchdog Timers, The Doc says to wait for reg offset 0x44, 
	 * it seems only offset 0x34 works
	 */
	out32(OMAP3530_WDT2_BASE + 0x48, 0xAAAA);
	while (in32(OMAP3530_WDT2_BASE + 0x34) != 0);
	out32(OMAP3530_WDT2_BASE + 0x48, 0x5555);
}

static unsigned pin_data[]={
/* SDRC */
    CONTROL_PADCONF_SDRC_D0,        (IEN  | PTD | PDIS | M0),   /*SDRC_D0*/
    CONTROL_PADCONF_SDRC_D1,        (IEN  | PTD | PDIS | M0),   /*SDRC_D1*/
    CONTROL_PADCONF_SDRC_D2,        (IEN  | PTD | PDIS | M0),   /*SDRC_D2*/
    CONTROL_PADCONF_SDRC_D3,        (IEN  | PTD | PDIS | M0),   /*SDRC_D3*/
    CONTROL_PADCONF_SDRC_D4,        (IEN  | PTD | PDIS | M0),   /*SDRC_D4*/
    CONTROL_PADCONF_SDRC_D5,        (IEN  | PTD | PDIS | M0),   /*SDRC_D5*/
    CONTROL_PADCONF_SDRC_D6,        (IEN  | PTD | PDIS | M0),   /*SDRC_D6*/
    CONTROL_PADCONF_SDRC_D7,        (IEN  | PTD | PDIS | M0),   /*SDRC_D7*/
    CONTROL_PADCONF_SDRC_D8,        (IEN  | PTD | PDIS | M0),   /*SDRC_D8*/
    CONTROL_PADCONF_SDRC_D9,        (IEN  | PTD | PDIS | M0),   /*SDRC_D9*/
    CONTROL_PADCONF_SDRC_D10,       (IEN  | PTD | PDIS | M0),   /*SDRC_D10*/
    CONTROL_PADCONF_SDRC_D11,       (IEN  | PTD | PDIS | M0),   /*SDRC_D11*/
    CONTROL_PADCONF_SDRC_D12,       (IEN  | PTD | PDIS | M0),   /*SDRC_D12*/
    CONTROL_PADCONF_SDRC_D13,       (IEN  | PTD | PDIS | M0),   /*SDRC_D13*/
    CONTROL_PADCONF_SDRC_D14,       (IEN  | PTD | PDIS | M0),   /*SDRC_D14*/
    CONTROL_PADCONF_SDRC_D15,       (IEN  | PTD | PDIS | M0),   /*SDRC_D15*/
    CONTROL_PADCONF_SDRC_D16,       (IEN  | PTD | PDIS | M0),   /*SDRC_D16*/
    CONTROL_PADCONF_SDRC_D17,       (IEN  | PTD | PDIS | M0),   /*SDRC_D17*/
    CONTROL_PADCONF_SDRC_D18,       (IEN  | PTD | PDIS | M0),   /*SDRC_D18*/
    CONTROL_PADCONF_SDRC_D19,       (IEN  | PTD | PDIS | M0),   /*SDRC_D19*/
    CONTROL_PADCONF_SDRC_D20,       (IEN  | PTD | PDIS | M0),   /*SDRC_D20*/
    CONTROL_PADCONF_SDRC_D21,       (IEN  | PTD | PDIS | M0),   /*SDRC_D21*/
    CONTROL_PADCONF_SDRC_D22,       (IEN  | PTD | PDIS | M0),   /*SDRC_D22*/
    CONTROL_PADCONF_SDRC_D23,       (IEN  | PTD | PDIS | M0),   /*SDRC_D23*/
    CONTROL_PADCONF_SDRC_D24,       (IEN  | PTD | PDIS | M0),   /*SDRC_D24*/
    CONTROL_PADCONF_SDRC_D25,       (IEN  | PTD | PDIS | M0),   /*SDRC_D25*/
    CONTROL_PADCONF_SDRC_D26,       (IEN  | PTD | PDIS | M0),   /*SDRC_D26*/
    CONTROL_PADCONF_SDRC_D27,       (IEN  | PTD | PDIS | M0),   /*SDRC_D27*/
    CONTROL_PADCONF_SDRC_D28,       (IEN  | PTD | PDIS | M0),   /*SDRC_D28*/
    CONTROL_PADCONF_SDRC_D29,       (IEN  | PTD | PDIS | M0),   /*SDRC_D29*/
    CONTROL_PADCONF_SDRC_D30,       (IEN  | PTD | PDIS | M0),   /*SDRC_D30*/
    CONTROL_PADCONF_SDRC_D31,       (IEN  | PTD | PDIS | M0),   /*SDRC_D31*/
    CONTROL_PADCONF_SDRC_CLK,       (IEN  | PTD | PDIS | M0),   /*SDRC_CLK*/
    CONTROL_PADCONF_SDRC_DQS0,      (IEN  | PTD | PDIS | M0),   /*SDRC_DQS0*/
    CONTROL_PADCONF_SDRC_DQS1,      (IEN  | PTD | PDIS | M0),   /*SDRC_DQS1*/
    CONTROL_PADCONF_SDRC_DQS2,      (IEN  | PTD | PDIS | M0),   /*SDRC_DQS2*/
    CONTROL_PADCONF_SDRC_DQS3,      (IEN  | PTD | PDIS | M0),   /*SDRC_DQS3*/
 /*GPMC*/
    CONTROL_PADCONF_GPMC_A1,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A1*/
    CONTROL_PADCONF_GPMC_A2,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A2*/
    CONTROL_PADCONF_GPMC_A3,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A3*/
    CONTROL_PADCONF_GPMC_A4,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A4*/
    CONTROL_PADCONF_GPMC_A5,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A5*/
    CONTROL_PADCONF_GPMC_A6,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A6*/
    CONTROL_PADCONF_GPMC_A7,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A7*/
    CONTROL_PADCONF_GPMC_A8,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A8*/
    CONTROL_PADCONF_GPMC_A9,        (IDIS | PTD | PDIS  | M0),   /*GPMC_A9*/
    CONTROL_PADCONF_GPMC_A10,       (IDIS | PTD | PDIS  | M0),   /*GPMC_A10*/
    CONTROL_PADCONF_GPMC_D0,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D0*/
    CONTROL_PADCONF_GPMC_D1,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D1*/
    CONTROL_PADCONF_GPMC_D2,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D2*/
    CONTROL_PADCONF_GPMC_D3,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D3*/
    CONTROL_PADCONF_GPMC_D4,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D4*/
    CONTROL_PADCONF_GPMC_D5,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D5*/
    CONTROL_PADCONF_GPMC_D6,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D6*/
    CONTROL_PADCONF_GPMC_D7,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D7*/
    CONTROL_PADCONF_GPMC_D8,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D8*/
    CONTROL_PADCONF_GPMC_D9,        (IEN  | PTD | PDIS  | M0),   /*GPMC_D9*/
    CONTROL_PADCONF_GPMC_D10,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D10*/
    CONTROL_PADCONF_GPMC_D11,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D11*/
    CONTROL_PADCONF_GPMC_D12,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D12*/
    CONTROL_PADCONF_GPMC_D13,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D13*/
    CONTROL_PADCONF_GPMC_D14,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D14*/
    CONTROL_PADCONF_GPMC_D15,       (IEN  | PTD | PDIS  | M0),   /*GPMC_D15*/
    CONTROL_PADCONF_GPMC_NCS0,      (IDIS | PTU | PEN  | M0),   /*GPMC_nCS0*/
    CONTROL_PADCONF_GPMC_NCS1,      (IDIS | PTU | PEN  | M0),   /*GPMC_nCS1*/
    CONTROL_PADCONF_GPMC_NCS2,      (IDIS | PTU | PEN  | M0),   /*GPMC_nCS2*/
    CONTROL_PADCONF_GPMC_NCS3,      (IDIS | PTU | PEN  | M0),   /*GPMC_nCS3*/
    CONTROL_PADCONF_GPMC_NCS4,      (IDIS | PTU | PEN  | M0),   /*GPMC_nCS4*/
    CONTROL_PADCONF_GPMC_NCS5,      (IDIS | PTD | PDIS  | M0),   /*GPMC_nCS5*/
    CONTROL_PADCONF_GPMC_NCS6,      (IEN  | PTD | PDIS | M1),   /*GPMC_nCS6*/
    CONTROL_PADCONF_GPMC_NCS7,      (IEN  | PTU | PEN  | M1),   /*GPMC_nCS7*/
    CONTROL_PADCONF_GPMC_NBE1,      (IEN  | PTD | PDIS  | M0),   /*GPMC_nBE1*/
    CONTROL_PADCONF_GPMC_WAIT2,      (IEN  | PTU | PEN  | M0),   /*GPMC_wait2*/
    CONTROL_PADCONF_GPMC_WAIT3,      (IEN  | PTU | PEN  | M0),   /*GPMC_wait3*/
    CONTROL_PADCONF_GPMC_CLK,       (IDIS | PTD | PDIS  | M0),   /*GPMC_CLK*/
    CONTROL_PADCONF_GPMC_NADV_ALE,  (IDIS | PTD | PDIS | M0),   /*GPMC_nADV_ALE*/
    CONTROL_PADCONF_GPMC_NOE,       (IDIS | PTD | PDIS | M0),   /*GPMC_nOE*/
    CONTROL_PADCONF_GPMC_NWE,       (IDIS | PTD | PDIS | M0),   /*GPMC_nWE*/
    CONTROL_PADCONF_GPMC_NBE0_CLE,  (IDIS | PTD | PDIS  | M0),   /*GPMC_nBE0_CLE*/
    CONTROL_PADCONF_GPMC_NWP,       (IEN  | PTD | PDIS | M0),   /*GPMC_nWP*/
    CONTROL_PADCONF_GPMC_WAIT0,     (IEN  | PTU | PEN  | M0),   /*GPMC_WAIT0*/
    CONTROL_PADCONF_GPMC_WAIT1,     (IEN  | PTU | PEN  | M0),   /*GPMC_WAIT1*/

 /*DSS*/
    CONTROL_PADCONF_DSS_PCLK,       (IDIS | PTD | PDIS | M0),   /*DSS_PCLK*/
    CONTROL_PADCONF_DSS_HSYNC,      (IDIS | PTD | PDIS | M0),   /*DSS_HSYNC*/
    CONTROL_PADCONF_DSS_VSYNC,      (IDIS | PTD | PDIS | M0),   /*DSS_VSYNC*/
    CONTROL_PADCONF_DSS_ACBIAS,     (IDIS | PTD | PDIS | M0),   /*DSS_ACBIAS*/
    CONTROL_PADCONF_DSS_DATA0,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA0*/
    CONTROL_PADCONF_DSS_DATA1,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA1*/
    CONTROL_PADCONF_DSS_DATA2,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA2*/
    CONTROL_PADCONF_DSS_DATA3,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA3*/
    CONTROL_PADCONF_DSS_DATA4,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA4*/
    CONTROL_PADCONF_DSS_DATA5,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA5*/
    CONTROL_PADCONF_DSS_DATA6,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA6*/
    CONTROL_PADCONF_DSS_DATA7,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA7*/
    CONTROL_PADCONF_DSS_DATA8,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA8*/
    CONTROL_PADCONF_DSS_DATA9,      (IDIS | PTD | PDIS | M0),   /*DSS_DATA9*/
    CONTROL_PADCONF_DSS_DATA10,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA10*/
    CONTROL_PADCONF_DSS_DATA11,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA11*/
    CONTROL_PADCONF_DSS_DATA12,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA12*/
    CONTROL_PADCONF_DSS_DATA13,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA13*/
    CONTROL_PADCONF_DSS_DATA14,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA14*/
    CONTROL_PADCONF_DSS_DATA15,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA15*/
    CONTROL_PADCONF_DSS_DATA16,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA16*/
    CONTROL_PADCONF_DSS_DATA17,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA17*/
    CONTROL_PADCONF_DSS_DATA18,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA18*/
    CONTROL_PADCONF_DSS_DATA19,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA19*/
    CONTROL_PADCONF_DSS_DATA20,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA20*/
    CONTROL_PADCONF_DSS_DATA21,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA21*/
    CONTROL_PADCONF_DSS_DATA22,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA22*/
    CONTROL_PADCONF_DSS_DATA23,     (IDIS | PTD | PDIS | M0),   /*DSS_DATA23*/
 /*CAMERA*/
    CONTROL_PADCONF_CAM_HS,         (IEN  | PTU | PEN  | M0),   /*CAM_HS*/
    CONTROL_PADCONF_CAM_VS,         (IEN  | PTU | PEN  | M0),   /*CAM_VS*/
    CONTROL_PADCONF_CAM_XCLKA,      (IDIS | PTD | PDIS | M0),   /*CAM_XCLKA*/
    CONTROL_PADCONF_CAM_PCLK,       (IEN  | PTU | PEN  | M0),   /*CAM_PCLK*/
    CONTROL_PADCONF_CAM_FLD,        (IDIS | PTD | PDIS | M4),   /*GPIO_98, CAM_RESET*/
    CONTROL_PADCONF_CAM_D0,         (IEN  | PTD | PDIS | M0),   /*CAM_D0*/
    CONTROL_PADCONF_CAM_D1,         (IEN  | PTD | PDIS | M0),   /*CAM_D1*/
    CONTROL_PADCONF_CAM_D2,         (IEN  | PTD | PDIS | M0),   /*CAM_D2*/
    CONTROL_PADCONF_CAM_D3,         (IEN  | PTD | PDIS | M0),   /*CAM_D3*/
    CONTROL_PADCONF_CAM_D4,         (IEN  | PTD | PDIS | M0),   /*CAM_D4*/
    CONTROL_PADCONF_CAM_D5,         (IEN  | PTD | PDIS | M0),   /*CAM_D5*/
    CONTROL_PADCONF_CAM_D6,         (IEN  | PTD | PDIS | M0),   /*CAM_D6*/
    CONTROL_PADCONF_CAM_D7,         (IEN  | PTD | PDIS | M0),   /*CAM_D7*/
    CONTROL_PADCONF_CAM_D8,         (IEN  | PTD | PDIS | M0),   /*CAM_D8*/
    CONTROL_PADCONF_CAM_D9,         (IEN  | PTD | PDIS | M0),   /*CAM_D9*/
    CONTROL_PADCONF_CAM_D10,        (IEN  | PTD | PDIS | M0),   /*CAM_D10*/
    CONTROL_PADCONF_CAM_D11,        (IEN  | PTD | PDIS | M0),   /*CAM_D11*/
    CONTROL_PADCONF_CAM_XCLKB,      (IDIS | PTD | PDIS | M0),   /*CAM_XCLKB*/
    CONTROL_PADCONF_CAM_WEN,        (IEN  | PTD | PDIS | M4),   /*GPIO_167*/
    CONTROL_PADCONF_CAM_STROBE,     (IDIS | PTD | PDIS | M0),   /*CAM_STROBE*/
    CONTROL_PADCONF_CSI2_DX0,       (IEN  | PTD | PDIS | M0),   /*CSI2_DX0*/
    CONTROL_PADCONF_CSI2_DY0,       (IEN  | PTD | PDIS | M0),   /*CSI2_DY0*/
    CONTROL_PADCONF_CSI2_DX1,       (IEN  | PTD | PDIS | M0),   /*CSI2_DX1*/
    CONTROL_PADCONF_CSI2_DY1,       (IEN  | PTD | PDIS | M0),   /*CSI2_DY1*/
 /*Audio Interface */
    CONTROL_PADCONF_MCBSP2_FSX,     (IEN  | PTD | PDIS | M0),   /*McBSP2_FSX*/
    CONTROL_PADCONF_MCBSP2_CLKX,    (IEN  | PTD | PDIS | M0),   /*McBSP2_CLKX*/
    CONTROL_PADCONF_MCBSP2_DR,      (IEN  | PTD | PDIS | M0),   /*McBSP2_DR*/
    CONTROL_PADCONF_MCBSP2_DX,      (IDIS | PTD | PDIS | M0),   /*McBSP2_DX*/
 /*Expansion card  */
    CONTROL_PADCONF_MMC1_CLK,       (IDIS  | PTU | PEN  | M0),   /*MMC1_CLK*/
    CONTROL_PADCONF_MMC1_CMD,       (IEN  | PTU | PEN | M0),   /*MMC1_CMD*/
    CONTROL_PADCONF_MMC1_DAT0,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT0*/
    CONTROL_PADCONF_MMC1_DAT1,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT1*/
    CONTROL_PADCONF_MMC1_DAT2,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT2*/
    CONTROL_PADCONF_MMC1_DAT3,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT3*/
    CONTROL_PADCONF_MMC1_DAT4,      (IEN  | PTU | PEN  | M0),   /*MMC1_DAT4, WriteProtect*/
    CONTROL_PADCONF_MMC1_DAT5,      (IEN  | PTU | PEN  | M0),   /*MMC1_DAT5, CardDetect*/
    CONTROL_PADCONF_MMC1_DAT6,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT6*/
    CONTROL_PADCONF_MMC1_DAT7,      (IEN  | PTU | PEN | M0),   /*MMC1_DAT7*/
 /*Wireless LAN */
    CONTROL_PADCONF_MMC2_CLK,       (IEN  | PTU | PEN | M4),   /*GPIO_130*/
    CONTROL_PADCONF_MMC2_CMD,       (IEN  | PTU | PEN  | M4),   /*GPIO_131*/
    CONTROL_PADCONF_MMC2_DAT0,      (IEN  | PTU | PEN  | M4),   /*GPIO_132*/
    CONTROL_PADCONF_MMC2_DAT1,      (IEN  | PTU | PEN  | M4),   /*GPIO_133*/
    CONTROL_PADCONF_MMC2_DAT2,      (IEN  | PTU | PEN  | M4),   /*GPIO_134*/
    CONTROL_PADCONF_MMC2_DAT3,      (IEN  | PTU | PEN  | M4),   /*GPIO_135*/
    CONTROL_PADCONF_MMC2_DAT4,      (IEN | PTU | PEN | M4),   /*GPIO_136*/
    CONTROL_PADCONF_MMC2_DAT5,      (IEN | PTU | PEN | M4),   /*GPIO_137*/
    CONTROL_PADCONF_MMC2_DAT6,      (IEN | PTU | PEN | M4),   /*GPIO_138*/
    CONTROL_PADCONF_MMC2_DAT7,      (IEN  | PTU | PEN  | M4),   /*GPIO_139*/
 /*Bluetooth*/
    CONTROL_PADCONF_MCBSP3_DX,      (IEN | PTD | PDIS | M4),   /*GPIO_140*/
    CONTROL_PADCONF_MCBSP3_DR,      (IEN  | PTD | PDIS | M4),   /*GPIO_141*/
    CONTROL_PADCONF_MCBSP3_CLKX,    (IEN  | PTD | PDIS | M4),   /*GPIO_142*/
    CONTROL_PADCONF_MCBSP3_FSX,     (IEN | PTD | PDIS | M1),   /*GPIO_143*/
    CONTROL_PADCONF_UART2_CTS,      (IEN  | PTU | PEN  | M0),   /*UART2_CTS*/
    CONTROL_PADCONF_UART2_RTS,      (IDIS | PTD | PDIS | M0),   /*UART2_RTS*/
    CONTROL_PADCONF_UART2_TX,       (IDIS | PTD | PDIS | M0),   /*UART2_TX*/
    CONTROL_PADCONF_UART2_RX,       (IEN  | PTD | PDIS | M4),   /*UART2_RX*/
 /*Modem Interface */
    CONTROL_PADCONF_UART1_TX,       (IDIS | PTD | PDIS | M0),   /*UART1_TX*/
    CONTROL_PADCONF_UART1_RTS,      (IDIS | PTD | PDIS | M4),   /*GPIO_149*/
    CONTROL_PADCONF_UART1_CTS,      (IDIS  | PTU | PDIS | M4),   /*GPIO_150*/
    CONTROL_PADCONF_UART1_RX,       (IEN  | PTD | PDIS | M0),   /*UART1_RX*/
    CONTROL_PADCONF_MCBSP4_CLKX,    (IEN | PTD | PDIS | M1),   /*SSI1_DAT_RX*/
    CONTROL_PADCONF_MCBSP4_DR,      (IEN | PTD | PDIS | M1),   /*SSI1_FLAG_RX*/
    CONTROL_PADCONF_MCBSP4_DX,      (IEN | PTD | PDIS | M1),   /*SSI1_RDY_RX*/
    CONTROL_PADCONF_MCBSP4_FSX,     (IEN | PTD | PDIS | M1),   /*SSI1_WAKE*/
    CONTROL_PADCONF_MCBSP1_CLKR,    (IDIS  | PTD | PDIS | M4),   /*GPIO_156*/
    CONTROL_PADCONF_MCBSP1_FSR,     (IDIS | PTU | PEN  | M4),   /*GPIO_157*/
    CONTROL_PADCONF_MCBSP1_DX,      (IDIS | PTD | PDIS | M4),   /*GPIO_158*/
    CONTROL_PADCONF_MCBSP1_DR,      (IDIS  | PTD | PDIS | M4),   /*GPIO_159*/
    CONTROL_PADCONF_MCBSP_CLKS,     (IEN  | PTU | PDIS | M0),   /*MCBSP_CLKS*/
    CONTROL_PADCONF_MCBSP1_FSX,     (IDIS  | PTD | PDIS | M4),   /*GPIO_161*/
    CONTROL_PADCONF_MCBSP1_CLKX,    (IDIS  | PTD | PDIS | M4),   /*GPIO_162*/
 /*Serial Interface*/
    CONTROL_PADCONF_UART3_CTS_RCTX, (IEN  | PTU | PEN | M0),   /*UART3_CTS_RCTX*/
    CONTROL_PADCONF_UART3_RTS_SD,   (IDIS | PTD | PDIS | M0),   /*UART3_RTS_SD*/
    CONTROL_PADCONF_UART3_RX_IRRX,  (IEN  | PTD | PDIS | M0),   /*UART3_RX_IRRX*/
    CONTROL_PADCONF_UART3_TX_IRTX,  (IDIS | PTD | PDIS | M0),   /*UART3_TX_IRTX*/
    CONTROL_PADCONF_HSUSB0_CLK,     (IEN  | PTD | PDIS | M0),   /*HSUSB0_CLK*/
    CONTROL_PADCONF_HSUSB0_STP,     (IDIS | PTU | PEN  | M0),   /*HSUSB0_STP*/
    CONTROL_PADCONF_HSUSB0_DIR,     (IEN  | PTD | PDIS | M0),   /*HSUSB0_DIR*/
    CONTROL_PADCONF_HSUSB0_NXT,     (IEN  | PTD | PDIS | M0),   /*HSUSB0_NXT*/
    CONTROL_PADCONF_HSUSB0_DATA0,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA0*/
    CONTROL_PADCONF_HSUSB0_DATA1,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA1*/
    CONTROL_PADCONF_HSUSB0_DATA2,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA2*/
    CONTROL_PADCONF_HSUSB0_DATA3,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA3*/
    CONTROL_PADCONF_HSUSB0_DATA4,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA4*/
    CONTROL_PADCONF_HSUSB0_DATA5,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA5*/
    CONTROL_PADCONF_HSUSB0_DATA6,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA6*/
    CONTROL_PADCONF_HSUSB0_DATA7,   (IEN  | PTD | PDIS | M0),   /*HSUSB0_DATA7*/
    CONTROL_PADCONF_I2C1_SCL,       (IEN  | PTU | PEN  | M0),   /*I2C1_SCL*/
    CONTROL_PADCONF_I2C1_SDA,       (IEN  | PTU | PEN  | M0),   /*I2C1_SDA*/
    CONTROL_PADCONF_I2C2_SCL,       (IDIS  | PTU | PDIS  | M4),   /*GPIO_168*/
    CONTROL_PADCONF_I2C2_SDA,       (IEN  | PTU | PEN  | M4),   /*GPIO_183*/
    CONTROL_PADCONF_I2C3_SCL,       (IEN  | PTU | PEN  | M0),   /*I2C3_SCL*/
    CONTROL_PADCONF_I2C3_SDA,       (IEN  | PTU | PEN  | M0),   /*I2C3_SDA*/
    CONTROL_PADCONF_GPIO_170,		(IDIS  | PTU | PEN  | M4),   /*GPIO_170 for DVI-D*/
    CONTROL_PADCONF_I2C4_SCL,       (IEN  | PTU | PEN  | M0),   /*I2C4_SCL*/
    CONTROL_PADCONF_I2C4_SDA,       (IEN  | PTU | PEN  | M0),   /*I2C4_SDA*/
    CONTROL_PADCONF_HDQ_SIO,        (IDIS  | PTU | PEN  | M4),   /*HDQ_SIO*/
    CONTROL_PADCONF_MCSPI1_CLK,     (IEN  | PTD | PDIS | M0),   /*McSPI1_CLK*/
    CONTROL_PADCONF_MCSPI1_SIMO,    (IEN  | PTD | PDIS | M0),   /*McSPI1_SIMO*/
    CONTROL_PADCONF_MCSPI1_SOMI,    (IEN  | PTD | PDIS | M0),   /*McSPI1_SOMI*/
    CONTROL_PADCONF_MCSPI1_CS0,     (IEN  | PTD | PEN  | M0),   /*McSPI1_CS0*/
    CONTROL_PADCONF_MCSPI1_CS1,     (IDIS  | PTD | PEN  | M0),   /*McSPI1_CS1*/
    CONTROL_PADCONF_MCSPI1_CS2,     (IDIS  | PTD | PDIS | M4),   /*GPIO_176*/
    CONTROL_PADCONF_MCSPI1_CS3,     (IEN  | PTD | PEN  | M0),   /*McSPI1_CS3*/
    CONTROL_PADCONF_MCSPI2_CLK,     (IEN  | PTD | PDIS | M0),   /*McSPI2_CLK*/
    CONTROL_PADCONF_MCSPI2_SIMO,    (IEN  | PTD | PDIS | M0),   /*McSPI2_SIMO*/
    CONTROL_PADCONF_MCSPI2_SOMI,    (IEN  | PTD | PDIS | M0),   /*McSPI2_SOMI*/
    CONTROL_PADCONF_MCSPI2_CS0,     (IEN  | PTD | PEN  | M0),   /*McSPI2_CS0*/
    CONTROL_PADCONF_MCSPI2_CS1,     (IEN  | PTD | PEN  | M0),   /*McSPI2_CS1*/
 /*Control and debug */
    CONTROL_PADCONF_SYS_32K,        (IEN  | PTD | PDIS | M0),   /*SYS_32K*/
    CONTROL_PADCONF_SYS_CLKREQ,     (IEN  | PTD | PDIS | M0),   /*SYS_CLKREQ*/
    CONTROL_PADCONF_SYS_NIRQ,       (IEN  | PTU | PEN  | M0),   /*SYS_nIRQ*/
    CONTROL_PADCONF_SYS_BOOT0,      (IEN  | PTD | PDIS | M4),   /*GPIO_2, PEN_IRQ*/
    CONTROL_PADCONF_SYS_BOOT1,      (IEN  | PTD | PDIS | M4),   /*GPIO_3*/
    CONTROL_PADCONF_SYS_BOOT2,      (IEN  | PTD | PDIS | M4),   /*GPIO_4,MMC1_WP*/
    CONTROL_PADCONF_SYS_BOOT3,      (IEN  | PTD | PDIS | M4),   /*GPIO_5*/
    CONTROL_PADCONF_SYS_BOOT4,      (IEN  | PTD | PDIS | M4),   /*GPIO_6*/
    CONTROL_PADCONF_SYS_BOOT5,      (IEN  | PTD | PDIS | M4),   /*GPIO_7, MMC2_WP*/
    CONTROL_PADCONF_SYS_BOOT6,      (IDIS | PTD | PDIS | M4),   /*GPIO_8, LCD_ENBKL*/
    CONTROL_PADCONF_SYS_OFF_MODE,   (IEN  | PTD | PDIS | M0),   /*SYS_OFF_MODE*/
    CONTROL_PADCONF_SYS_CLKOUT1,    (IDIS  | PTD | PEN | M0),   /*SYS_CLKOUT1*/
    CONTROL_PADCONF_SYS_CLKOUT2,    (IEN  | PTU | PEN  | M4),   /*GPIO_186*/
    CONTROL_PADCONF_ETK_CLK_ES2,    (IDIS | PTU | PDIS  | M3),   /*HSUSB1_STP*/
    CONTROL_PADCONF_ETK_CTL_ES2,    (IDIS | PTU | PEN | M3),   /*HSUSB1_CLK*/
    CONTROL_PADCONF_ETK_D0_ES2 ,    (IDIS | PTU | PEN | M3),   /*HSUSB1_DATA0*/
    CONTROL_PADCONF_ETK_D1_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA1*/
    CONTROL_PADCONF_ETK_D2_ES2 ,    (IDIS  | PTU | PEN  | M3),   /*HSUSB1_DATA2*/
    CONTROL_PADCONF_ETK_D3_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA7*/
    CONTROL_PADCONF_ETK_D4_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA4*/
    CONTROL_PADCONF_ETK_D5_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA5*/
    CONTROL_PADCONF_ETK_D6_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA6*/
    CONTROL_PADCONF_ETK_D7_ES2 ,    (IDIS  | PTU | PEN | M3),   /*HSUSB1_DATA3*/
    CONTROL_PADCONF_ETK_D8_ES2 ,    (IEN  | PTD | PDIS | M3),   /*HSUSB1_DIR*/
    CONTROL_PADCONF_ETK_D9_ES2 ,    (IEN  | PTD | PDIS | M3),   /*HSUSB1_NXT*/
    CONTROL_PADCONF_ETK_D10_ES2,    (IDIS  | PTU | PEN | M4),   /*GPIO_24*/
    CONTROL_PADCONF_ETK_D15,    		(IEN  | PTU | PEN | M4),   /*GPIO_29*/
 /*Die to Die */
     CONTROL_PADCONF_D2D_MCAD34,     (IEN  | PTD | PEN  | M0),   /*d2d_mcad34*/
    CONTROL_PADCONF_D2D_MCAD35,     (IEN  | PTD | PEN  | M0),   /*d2d_mcad35*/
    CONTROL_PADCONF_D2D_MCAD36,     (IEN  | PTD | PEN  | M0),   /*d2d_mcad36*/
    CONTROL_PADCONF_D2D_CLK26MI,    (IEN  | PTD | PDIS | M0),   /*d2d_clk26mi*/
    CONTROL_PADCONF_D2D_NRESPWRON,  (IEN  | PTD | PEN  | M0),   /*d2d_nrespwron*/
    CONTROL_PADCONF_D2D_NRESWARM,   (IEN  | PTU | PEN  | M0),   /*d2d_nreswarm */
    CONTROL_PADCONF_D2D_ARM9NIRQ,   (IEN  | PTD | PDIS | M0),   /*d2d_arm9nirq */
    CONTROL_PADCONF_D2D_UMA2P6FIQ,  (IEN  | PTD | PDIS | M0),   /*d2d_uma2p6fiq*/
    CONTROL_PADCONF_D2D_SPINT,      (IEN  | PTD | PEN  | M0),   /*d2d_spint*/
    CONTROL_PADCONF_D2D_FRINT,      (IEN  | PTD | PEN  | M0),   /*d2d_frint*/
    CONTROL_PADCONF_D2D_DMAREQ0,    (IEN  | PTD | PDIS | M0),   /*d2d_dmareq0*/
    CONTROL_PADCONF_D2D_DMAREQ1,    (IEN  | PTD | PDIS | M0),   /*d2d_dmareq1*/
    CONTROL_PADCONF_D2D_DMAREQ2,    (IEN  | PTD | PDIS | M0),   /*d2d_dmareq2*/
    CONTROL_PADCONF_D2D_DMAREQ3,    (IEN  | PTD | PDIS | M0),   /*d2d_dmareq3*/
    CONTROL_PADCONF_D2D_N3GTRST,    (IEN  | PTD | PDIS | M0),   /*d2d_n3gtrst*/
    CONTROL_PADCONF_D2D_N3GTDI,     (IEN  | PTD | PDIS | M0),   /*d2d_n3gtdi*/
    CONTROL_PADCONF_D2D_N3GTDO,     (IEN  | PTD | PDIS | M0),   /*d2d_n3gtdo*/
    CONTROL_PADCONF_D2D_N3GTMS,     (IEN  | PTD | PDIS | M0),   /*d2d_n3gtms*/
    CONTROL_PADCONF_D2D_N3GTCK,     (IEN  | PTD | PDIS | M0),   /*d2d_n3gtck*/
    CONTROL_PADCONF_D2D_N3GRTCK,    (IEN  | PTD | PDIS | M0),   /*d2d_n3grtck*/
    CONTROL_PADCONF_D2D_MSTDBY,     (IEN  | PTU | PEN  | M0),   /*d2d_mstdby*/
    CONTROL_PADCONF_D2D_SWAKEUP,    (IEN  | PTD | PEN  | M0),   /*d2d_swakeup*/
    CONTROL_PADCONF_D2D_IDLEREQ,    (IEN  | PTD | PDIS | M0),   /*d2d_idlereq*/
    CONTROL_PADCONF_D2D_IDLEACK,    (IEN  | PTU | PEN  | M0),   /*d2d_idleack*/
    CONTROL_PADCONF_D2D_MWRITE,     (IEN  | PTD | PDIS | M0),   /*d2d_mwrite*/
    CONTROL_PADCONF_D2D_SWRITE,     (IEN  | PTD | PDIS | M0),   /*d2d_swrite*/
    CONTROL_PADCONF_D2D_MREAD,      (IEN  | PTD | PDIS | M0),   /*d2d_mread*/
    CONTROL_PADCONF_D2D_SREAD,      (IEN  | PTD | PDIS | M0),   /*d2d_sread*/
    CONTROL_PADCONF_D2D_MBUSFLAG,   (IEN  | PTD | PDIS | M0),   /*d2d_mbusflag*/
    CONTROL_PADCONF_D2D_SBUSFLAG,   (IEN  | PTD | PDIS | M0),   /*d2d_sbusflag*/
    CONTROL_PADCONF_SDRC_CKE0,      (IDIS | PTU | PEN | M0),   /*sdrc_cke0*/
    CONTROL_PADCONF_SDRC_CKE1,      (IDIS | PTU | PEN | M0)    /*sdrc_cke1*/
};

static void init_mux(void)
{
	int i, size;
	unsigned pad;
	size = sizeof(pin_data)/sizeof(unsigned);
	for(i=0; i<size; i+=2){
		     out16(pin_data[i], pin_data[i+1]);
	}

	/** Init MMC **/
	pad = in32(CONTROL_PBIAS_LITE);
	out32(CONTROL_PBIAS_LITE,  pad | (1 << 2) | (1 << 1) | (1 << 9));

	pad = in32(CONTROL_DEVCONF0); //enable pad for MMC, external clk for McBSP1,2
	out32(CONTROL_DEVCONF0, pad | MMCSDIO1ADPCLKISEL|MCBSP1_CLKS|MCBSP2_CLKS);

	pad = in32(CONTROL_DEVCONF1); //external clk for McBSP3
	out32(CONTROL_DEVCONF1, pad | MCBSP3_CLKS);

}

static void init_gpmc(void)
{
	unsigned base = OMAP3530_GPMC_BASE;

	out32(base + GPMC_IRQENABLE, 0);/* isr's sources masked */
	out32(base + GPMC_TIMEOUT_CONTROL, 0);/* timeout disable */
	out32(base + GPMC_CONFIG, 0x13); /* disable WP, all WAIT pin active low, and  */

	/* disable all chip select */
	out32(base + GPMC_CONFIG7, 0);
	delay(1000);

	/*CS0 -- Boot Flash 16 bit NAND, start address, 0x08000000 */
	out32(base + GPMC_CONFIG7, (0xF<<8)|(1<<6)| 0x8);
	delay(100);

	out32(base + GPMC_CONFIG1, M_NAND_GPMC_CONFIG1);
	out32(base + GPMC_CONFIG2, M_NAND_GPMC_CONFIG2);
	out32(base + GPMC_CONFIG3, M_NAND_GPMC_CONFIG3);
	out32(base + GPMC_CONFIG4, M_NAND_GPMC_CONFIG4);
	out32(base + GPMC_CONFIG5, M_NAND_GPMC_CONFIG5);
	out32(base + GPMC_CONFIG6, M_NAND_GPMC_CONFIG6);
	out32(base + GPMC_CONFIG7, (0x8<<8)|(1<<6)| 0x30);

	delay(1000);
}


uint32_t mem_ok(uint32_t cs)
{
        uint32_t val1, val2, addr;
        uint32_t pattern = 0x12345678;

        if (cs == 0)
	        addr = OMAP34XX_SDRC_CS0;
	    else
	        addr = OMAP34XX_SDRC_CS1;
	        
        out32(addr + 0x400, 0x0);      /* clear pos A */
        out32(addr, pattern);          /* pattern to pos B */
        out32(addr + 4, 0x0);          /* remove pattern off the bus */
        val1 = in32(addr + 0x400);     /* get pos A value */
        val2 = in32(addr);             /* get val2 */
        if ((val1 != 0) || (val2 != pattern))   /* see if pos A val changed */
                return 0;
        else
                return 1;
}


void init_sdram()
{
	unsigned sdrc_base = OMAP3530_SDRC_BASE;

	/* reset sdrc controller */
	out32(sdrc_base + SDRC_SYSCONFIG, SOFTRESET);
	while ((in32(sdrc_base + SDRC_SYSSTATUS) & (1 << 0)) == 0);
	out32(sdrc_base + SDRC_SYSCONFIG, 0x0);

	/* setup sdrc to ball mux */
	out32(sdrc_base + SDRC_SHARING, SHARING);

	/* Disable Power Down of CKE cuz of 1 CKE on combo part */
	out32(sdrc_base + SDRC_POWER_REG , WAKEUPPROC | PWDNEN | SRFRONRESET | PAGEPOLICY_HIGH);

	out32(sdrc_base + SDRC_DLLA_CTRL, ENADLL | DLLPHASE_90);
	delay(20000);

    /********** configure cs0 *********/	
	/* SDRC_MCFG0 register */
	out32(sdrc_base + SDRC_MCFG_0 ,RASWIDTH_13BITS | CASWIDTH_10BITS | ADDRMUXLEGACY |
                RAMSIZE_128 | BANKALLOCATION | B32NOT16 | B32NOT16 |DEEPPD | DDR_SDRAM);

	/* SDRC_RFR_CTRL0 register */
	out32(sdrc_base + SDRC_RFR_CTRL_0, ARCV | ARE_ARCV_1);

	/* SDRC_ACTIM_CTRLA0 register */
	out32(sdrc_base + SDRC_ACTIM_CTRLA_0 ,MICRON_V_ACTIMA_165);

	/* SDRC_ACTIM_CTRLB0 register */
	out32(sdrc_base + SDRC_ACTIM_CTRLB_0 ,MICRON_V_ACTIMB_165);

	/* SDRC_Manual command register */
	out32(sdrc_base + SDRC_MANUAL_0 ,CMD_NOP); // NOP command
	out32(sdrc_base + SDRC_MANUAL_0 ,CMD_PRECHARGE); // Precharge command
	out32(sdrc_base + SDRC_MANUAL_0 ,CMD_AUTOREFRESH); // Auto-refresh command
	out32(sdrc_base + SDRC_MANUAL_0 ,CMD_AUTOREFRESH); // Auto-refresh command

	/* SDRC MR0 register */
	out32(sdrc_base +  SDRC_MR_0  ,CASL3 | BURSTLENGTH4);  // Burst length =4
	// CAS latency = 3
	// Write Burst = Read Burst

	if (!mem_ok(0)){
		out32(sdrc_base + SDRC_MCFG_0 ,0);	
	}
    
    /******** configure cs1 *********/	
	/* SDRC_MCFG1 register */
	out32(sdrc_base + SDRC_MCFG_1 ,RASWIDTH_13BITS | CASWIDTH_10BITS | ADDRMUXLEGACY |
                RAMSIZE_128 | BANKALLOCATION | B32NOT16 | B32NOT16 |DEEPPD | DDR_SDRAM);

	/* SDRC_RFR_CTRL1 register */
	out32(sdrc_base + SDRC_RFR_CTRL_1, ARCV | ARE_ARCV_1);

	/* SDRC_ACTIM_CTRLA1 register */
	out32(sdrc_base + SDRC_ACTIM_CTRLA_1 ,MICRON_V_ACTIMA_165);

	/* SDRC_ACTIM_CTRLB1 register */
	out32(sdrc_base + SDRC_ACTIM_CTRLB_1 ,MICRON_V_ACTIMB_165);

	/* SDRC_Manual command register */
	out32(sdrc_base + SDRC_MANUAL_1 ,CMD_NOP); // NOP command
	out32(sdrc_base + SDRC_MANUAL_1 ,CMD_PRECHARGE); // Precharge command
	out32(sdrc_base + SDRC_MANUAL_1 ,CMD_AUTOREFRESH); // Auto-refresh command
	out32(sdrc_base + SDRC_MANUAL_1 ,CMD_AUTOREFRESH); // Auto-refresh command

	/* SDRC MR1 register */
	out32(sdrc_base +  SDRC_MR_1  ,CASL3 | BURSTLENGTH4);  // Burst length =4
	// CAS latency = 3
	// Write Burst = Read Burst

	/* SDRC  CS_CFGregister */
	out32(sdrc_base +  SDRC_CS_CFG  ,0x1);  

	if (!mem_ok(1)){
		out32(sdrc_base + SDRC_MCFG_1 ,0);	
	}
}


static void icache_enable(void)
{
	unsigned long value;

	/* get control reg. */
      __asm__ __volatile__("mrc	p15, 0, %0, c1, c0, 0   @ read control reg\n":"=r"(value)
      	::"memory");

	delay(100);
	value |= (1<<12);
	__asm__ __volatile__("mcr	p15, 0, %0, c1, c0, 0   @ write it back\n"::"r"(value)
      :     "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0   @ read it back\n":"=r"(value)
	  ::"memory");

}


static void l2cache_enable()
{
	unsigned long i;
	volatile unsigned int j;
	unsigned long value;

	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 0":"=r"(value));

	/* ES2 onwards we can disable/enable L2 ourselves */
	if ((value & 0xf) != 0) {
		__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1":"=r"(i));
		__asm__ __volatile__("orr %0, %0, #0x2":"=r"(i));
		__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1":"=r"(i));
	} else {
		/* Save r0, r12 and restore them after usage */
		__asm__ __volatile__("mov %0, r12":"=r"(j));
		__asm__ __volatile__("mov %0, r0":"=r"(i));

		/* GP Device ROM code API usage here */
		/* r12 = AUXCR Write function and r0 value */
		__asm__ __volatile__("mov r12, #0x3");
		__asm__ __volatile__("mrc p15, 0, r0, c1, c0, 1");
		__asm__ __volatile__("orr r0, r0, #0x2");
		/* SMI instruction to call ROM Code API */
		__asm__ __volatile__(".word 0xE1600070");
		__asm__ __volatile__("mov r0, %0":"=r"(i));
		__asm__ __volatile__("mov r12, %0":"=r"(j));
	}
}


static void dss_init()
{

		/*set GOIP_170 direction as output high*/
		out32(OMAP35XX_GPIO6_OE, in32(OMAP35XX_GPIO6_OE)& ~(1<<(170-OMAP35XX_GPIO6)));
		out32(OMAP35XX_GPIO6_DATAOUT, in32(OMAP35XX_GPIO6_DATAOUT) | (1<<(170-OMAP35XX_GPIO6)));
}

void init_omap3530beagle( )
{
	boot_from_flash = (in32(OMAP3530_SYSCTL_BASE+OMAP3530_SYSCTL_STATUS) & (1<<5))?0:1;
	
	init_watchdog();
	init_clocks();
	secure_unlock();
	v7_flush_dcache_all(get_device_type());
	icache_enable();
	l2cache_enable();
	setup_auxcr();
	init_mux();
	init_gpmc();
	init_sdram();
	dss_init();

	/* We use UART 3 as debug output */
	init_seromap(OMAP3530_UART3_BASE, 115200, 48000000, 16);
	//init nand
	nand_init();
	
}

__SRCVERSION( "$URL$ $Rev$" ); 

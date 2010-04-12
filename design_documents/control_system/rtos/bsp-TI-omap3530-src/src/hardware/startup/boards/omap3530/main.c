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




#include "startup.h"
#include <arm/omap.h>
#include <arm/omap3530.h>

#if 1	// SUNIL added to compile
extern struct callout_rtn	reboot_omap3530;
extern void	init_qtime_omap3530(void);
extern void	init_pinmux(void);
#endif

#define delay(x) {unsigned _delay = x; while (_delay--); }

int 	mtp;
int	pin_mux;
int	teb;



const struct callout_slot callouts[] = {
	{ CALLOUT_SLOT( reboot, _omap3530 ) },
};

const struct debug_device debug_devices[] = {
	{ 	"8250",
		{	"0x4806A000^2.0.48000000.16",	// UART 1, use the baud rate set by boot loader
		},
		init_omap,
		put_omap,
		{	&display_char_8250,
			&poll_key_8250,
			&break_detect_8250,
		}
	},
};

void
allocate_dsp_memory(paddr_t * resmem_addr, size_t * resmem_size, int count)
{
	int     i;

	for (i = 0; i < count; i++)
	{
		alloc_ram(resmem_addr[i], resmem_size[i], 1);
		hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_MISC, "misc", 0);
		hwi_add_location(resmem_addr[i], resmem_size[i], 0, hwi_find_as(resmem_addr[i], 0));
	}
}

void
allocate_dsplink_memory(paddr_t linkmem_addr, size_t linkmem_size)
{
	alloc_ram(linkmem_addr, linkmem_size, 1);
	hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_DMA, "misc", 0);
	hwi_add_location(linkmem_addr, linkmem_size, 0, hwi_find_as(linkmem_addr, 0));
}

unsigned long calc_omap_dpll(void) {

    unsigned dpll_mult = 0, dpll_div = 0;
    unsigned long long mn_output = 0, mx = 0, output = 0;
    unsigned long long sys_clkspeed = 0, processor_speed = 0;
    unsigned sys_clkin_sel = 0, osc_clkspeed = 0;
    unsigned dpll = 0, sys_clkdiv = 0;

    sys_clkin_sel = in32(OMAP35XX_PRM_CLKSEL) & 0x7;

	switch (sys_clkin_sel) {
	case 0:
		osc_clkspeed = 12000000;	/*12MHz*/
		break;
	case 1:
		osc_clkspeed = 13000000;	/*13MHz*/
		break;
	case 2:
		osc_clkspeed = 19200000;	/*19.2MHz*/
		break;
	case 3:
		osc_clkspeed = 26000000;	/*26MHz*/
		break;
	case 4:
		osc_clkspeed = 38400000;	/*38.4MHz*/
		break;
	}

    sys_clkdiv = (in32(OMAP35XX_PRM_CLKSRC_CTRL) >> 6) & 0x3;
    sys_clkspeed = osc_clkspeed / sys_clkdiv;

    dpll = in32(OMAP35XX_CM_CLKSEL1_PLL_MPU);
	mx = (in32(OMAP35XX_CM_CLKSEL2_PLL_MPU) & OMAP3530_MPU_DPLL_CLKOUT_DIV_MASK) >> OMAP3530_MPU_DPLL_CLKOUT_DIV_SHIFT;

    dpll_mult = dpll & OMAP3530_MPU_DPLL_MULT_MASK;
    dpll_mult >>= OMAP3530_MPU_DPLL_MULT_SHIFT;		/* 11 bits */

	dpll_div = dpll & OMAP3530_MPU_DPLL_DIV_MASK;
	dpll_div >>= OMAP3530_MPU_DPLL_DIV_SHIFT;		/* 7 bits */

	mn_output = (sys_clkspeed * dpll_mult);
    mn_output = (mn_output / (dpll_div + 1));

	/* Except for DPLL_M2 all clocks are (mn_output*2)/mx */
	output = (2 * mn_output) / mx;
	/* There is a divider in mpu which makes the actual
		processor speed half the dpll output */
	processor_speed = output / 2;

	return ((unsigned long) processor_speed);
}

/*
 * Enable clocks
 */
static void init_clocks(void)
{
	out32(OMAP35XX_CM_FCLKEN_IVA2, OMAP3530_FCK_IVA2_ON);
	out32(OMAP35XX_CM_FCLKEN1_CORE, OMAP3530_FCK_CORE1_ON);
	out32(OMAP35XX_CM_FCLKEN3_CORE, OMAP3530_FCK_CORE3_ON);
	out32(OMAP35XX_CM_ICLKEN1_CORE, OMAP3530_ICK_CORE1_ON);
	out32(OMAP35XX_CM_ICLKEN2_CORE, OMAP3530_ICK_CORE2_ON);
	out32(OMAP35XX_CM_ICLKEN3_CORE, OMAP3530_ICK_CORE3_ON);
	out32(OMAP35XX_CM_FCLKEN_WKUP, OMAP3530_FCK_WKUP_ON);
	out32(OMAP35XX_CM_ICLKEN_WKUP, OMAP3530_ICK_WKUP_ON);
	out32(OMAP35XX_CM_FCLKEN_DSS, OMAP3530_FCK_DSS_ON);
	out32(OMAP35XX_CM_ICLKEN_DSS, OMAP3530_ICK_DSS_ON);
	out32(OMAP35XX_CM_FCLKEN_CAM, OMAP3530_FCK_CAM_ON);
	out32(OMAP35XX_CM_ICLKEN_CAM, OMAP3530_ICK_CAM_ON);
	out32(OMAP35XX_CM_FCLKEN_PER, OMAP3530_FCK_PER_ON);
	out32(OMAP35XX_CM_ICLKEN_PER, OMAP3530_ICK_PER_ON);
	/* Set SGX clock ratio */
	out32(OMAP35XX_CM_CLKSEL_GFX, 0);
}

/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int		opt;
	char   *p;
	paddr_t resmem_addr[32];
	size_t  resmem_size[32];
	int     dsp_mem_count = 0;
	paddr_t linkmem_addr = 0;
	size_t  linkmem_size = 0;
	int     link_present = 0;

	pin_mux = 0;
	mtp = 0;
	teb = 0;

	init_clocks();

	add_callout_array(callouts, sizeof(callouts));

	while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING "x:L:spt")) != -1)
	{
		switch (opt)
		{
		case 'x':
			resmem_addr[dsp_mem_count] = getsize(optarg, &p);
			if (*p == ',')
			{
				resmem_size[dsp_mem_count] = getsize(p + 1, &p);
			}
			dsp_mem_count++;
			break;
		case 'L':
			linkmem_addr = getsize(optarg, &p);
			if (*p == ',')
			{
				linkmem_size = getsize(p + 1, &p);
			}
			link_present = 1;
			break;
		case 'p':
			pin_mux = 1;
			break;
		case 't':
			teb = 1;
			break;
		case 's':
			mtp = 1;
				break;
		default:
			handle_common_option(opt);
			break;
		}
	}

	/*
	 * Initialize debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	/*
	 * Collect information on all free RAM in the system
	 */
	init_raminfo();

	/* These allocations MUST be completed in this order */
	if (link_present)
		allocate_dsplink_memory(linkmem_addr, linkmem_size);

	if (dsp_mem_count > 0)
		allocate_dsp_memory(resmem_addr, resmem_size, dsp_mem_count);
	/* End allocations */

	/*
	 * Set CPU frequency
	 */
	if (cpu_freq == 0) {
		cpu_freq = calc_omap_dpll();
	}

	/*
	 * Remove RAM used by modules in the image
	 */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
		init_mmu();

	init_intrinfo();

	init_qtime_omap3530();

	init_cacheattr();

	init_cpuinfo();

	init_hwinfo();

	// pin_mux setting for beagle board
	if(pin_mux){
		init_pinmux();
	}

	add_typed_string(_CS_MACHINE, "OMAP3530");

	/*
	 * Load bootstrap executables in the image file system and Initialise
	 * various syspage pointers. This must be the _last_ initialisation done
	 * before transferring control to the next program.
	 */
	init_system_private();

	/*
	 * This is handy for debugging a new version of the startup program.
	 * Commenting this line out will save a great deal of code.
	 */
	print_syspage();

	return 0;
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/startup/boards/omap3530/main.c $ $Rev: 249398 $" );

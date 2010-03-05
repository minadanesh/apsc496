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

// Module Description:  board specific interface

#include <sim_mmc.h>
#include <sim_omap3.h>


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

int bs_init(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	omap3_ext_t		*omap;
	CONFIG_INFO		*cfg;

	cfg = (CONFIG_INFO *)&hba->cfg;

	if (!cfg->NumIOPorts) {
		cfg->IOPort_Base[0]   = OMAP3_MMCHS1_BASE;
		cfg->IOPort_Length[0] = OMAP3_MMCHS_SIZE;
		cfg->IOPort_Base[1]   = OMAP3_DMA4_BASE;
		cfg->IOPort_Length[1] = OMAP3_DMA4_SIZE;
		cfg->NumIOPorts = 2;
	} else if (cfg->NumIOPorts < 2) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "OMAP3 MMCSD: DMA4 base address must be specified");
		return MMC_FAILURE;
	}

	if (!cfg->NumIRQs) {
		cfg->IRQRegisters[0] = OMAP3_MMCHS1_IRQ;
		cfg->NumIRQs = 1;
	}

	if (!cfg->NumDMAs) {
		cfg->DMALst[0] = (DMA4_MMC1_TX >> 1) & 0xFF;
		cfg->DMALst[1] = DMA4_MMC1_TX;	// DMA request line for MMC1 TX
		cfg->DMALst[2] = DMA4_MMC1_RX;	// DMA request line for MMC1 RX
		cfg->NumDMAs = 3;
	} else if (cfg->NumDMAs == 1) {
		cfg->DMALst[1] = DMA4_MMC1_TX;	// DMA request line for MMC1 TX
		cfg->DMALst[2] = DMA4_MMC1_RX;	// DMA request line for MMC1 RX
		cfg->NumDMAs = 3;
	} else if (cfg->NumDMAs < 3) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "OMAP3 MMCSD: DMA channel and Tx/Rx request line must be specified");
		return MMC_FAILURE;
	}

	if (omap3_attach(hba) != MMC_SUCCESS)
		return MMC_FAILURE;

	ext  = (SIM_MMC_EXT *)hba->ext;
	omap = (omap3_ext_t *)ext->handle;

	omap->mmc_clock = 96000000;

	return MMC_SUCCESS;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

int bs_dinit(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	omap3_ext_t		*omap;

	ext  = (SIM_MMC_EXT *)hba->ext;
	omap = (omap3_ext_t *)ext->handle;

	return (CAM_SUCCESS);
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devb/mmcsd/arm/omap3.le/sim_bs.c $ $Rev: 249398 $" );

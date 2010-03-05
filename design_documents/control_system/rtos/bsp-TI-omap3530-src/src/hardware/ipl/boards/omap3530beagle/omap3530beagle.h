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


#ifndef __BEAGLE_H__
#define __BAEGLE_H__

#include <arm/omap3530.h>

#define OMAP34XX_SDRC_BASE 	0x6D000000
#define OMAP34XX_SDRC_CS0       0x80000000
#define OMAP34XX_SDRC_CS1       0x88000000

#define SDRC_SYSCONFIG          0x10
#define SDRC_SYSSTATUS          0x14
#define SDRC_CS_CFG 		0x40
#define SDRC_SHARING     	0x44
#define SDRC_ERR_ADDR           0x48
#define SDRC_ERR_TYPE           0x4C
#define SDRC_DLLA_CTRL          0x60
#define SDRC_DLLA_STATUS        0x64
#define SDRC_POWER_REG          0x70
#define SDRC_MCFG_0             0x80
#define SDRC_MCFG_1             0xB0
#define SDRC_MR_0               0x84
#define SDRC_MR_1               0xB4
#define SDRC_EMR2_0		0x8C
#define SDRC_EMR2_1		0xBC
#define SDRC_ACTIM_CTRLA_0      0x9C
#define SDRC_ACTIM_CTRLA_1      0xC4
#define SDRC_ACTIM_CTRLB_0      0xA0
#define SDRC_ACTIM_CTRLB_1      0xC8
#define SDRC_RFR_CTRL_0         0xA4
#define SDRC_RFR_CTRL_1         0xD4
#define SDRC_MANUAL_0           0xA8
#define SDRC_MANUAL_1           0xD8

#define DLLPHASE_90             (0x1 << 1)
#define LOADDLL                 (0x1 << 2)
#define ENADLL                  (0x1 << 3)
#define DLL_DELAY_MASK          0xFF00
#define DLL_NO_FILTER_MASK      ((0x1 << 9) | (0x1 << 8))
#define PAGEPOLICY_HIGH         (0x1 << 0)
#define SRFRONRESET             (0x1 << 7)
#define PWDNEN                  (0x1 << 2)
#define WAKEUPPROC              (0x1 << 26)
#define SOFTRESET               (0x1 << 1)
#define SHARING    	        0x00000100
#define ARE_ARCV_1              (0x1 << 0)
#define ARCV                    (0x4e2 << 8) /* Autorefresh count */
#define DDR_SDRAM               (0x1 << 0)
#define DEEPPD                  (0x1 << 3)
#define B32NOT16                (0x1 << 4)
#define BANKALLOCATION          (0x2 << 6)
#define RAMSIZE_128             (0x40 << 8) /* RAM size in 2MB chunks */
#define ADDRMUXLEGACY           (0x1 << 19)
#define CASWIDTH_10BITS         (0x5 << 20)
#define RASWIDTH_13BITS         (0x2 << 24)
#define BURSTLENGTH4            (0x2 << 0)
#define CASL3                   (0x3 << 4)

/* Micron  (165MHz optimized) 6.06ns
 * ACTIMA
 *      TDAL = Twr/Tck + Trp/tck= 15/6 + 18 /6 = 2.5 + 3 = 5.5 -> 6
 *      TDPL (Twr)      = 15/6  = 2.5 -> 3
 *      TRRD            = 12/6  = 2
 *      TRCD            = 18/6  = 3
 *      TRP             = 18/6  = 3
 *      TRAS            = 42/6  = 7
 *      TRC             = 60/6  = 10
 *      TRFC            = 125/6 = 21
 * ACTIMB
 *      TWTR            = 1
 *      TCKE            = 1
 *      TXSR            = 138/6 = 23
 *      TXP             = 25/6  = 4.1 ~5
 */
#define MICRON_TDAL_165         6
#define MICRON_TDPL_165         3
#define MICRON_TRRD_165         2
#define MICRON_TRCD_165         3
#define MICRON_TRP_165          3
#define MICRON_TRAS_165         7
#define MICRON_TRC_165          10
#define MICRON_TRFC_165         21
#define MICRON_V_ACTIMA_165 	((MICRON_TRFC_165 << 27) |(MICRON_TRC_165 << 22) |(MICRON_TRAS_165 << 18) | (MICRON_TRP_165 << 15) |(MICRON_TRCD_165 << 12) |  (MICRON_TRRD_165 << 9) |(MICRON_TDPL_165 << 6) |  (MICRON_TDAL_165))
#define MICRON_TWTR_165         1
#define MICRON_TCKE_165         1
#define MICRON_XSR_165          23
#define MICRON_TXP_165          5
#define MICRON_V_ACTIMB_165 	((MICRON_TCKE_165 << 12) |(MICRON_XSR_165 << 0) | (MICRON_TXP_165 << 8) | (MICRON_TWTR_165 << 16))


#define CMD_NOP                 0x0
#define CMD_PRECHARGE           0x1
#define CMD_AUTOREFRESH        	0x2
#define CMD_ENTR_PWRDOWN     	0x3
#define CMD_EXIT_PWRDOWN     	0x4
#define CMD_ENTR_SRFRSH         0x5
#define CMD_CKE_HIGH            0x6
#define CMD_CKE_LOW             0x7

#define M_NAND_GPMC_CONFIG1     0x00001800
#define M_NAND_GPMC_CONFIG2     0x00141400
#define M_NAND_GPMC_CONFIG3     0x00141400
#define M_NAND_GPMC_CONFIG4     0x0F010F01
#define M_NAND_GPMC_CONFIG5     0x010C1414
#define M_NAND_GPMC_CONFIG6     0x1f0f0A80
#define M_NAND_GPMC_CONFIG7     0x00000C44

#endif  /* __BEAGLE_MUX_H__ */

__SRCVERSION( "$URL$ $Rev$" );

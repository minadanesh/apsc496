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


#define VARIANT_DSP

#define DEFAULT_MCBSP       		1
#define MCBSP_PLAYBACK_PATH 		ENABLED
#define MCBSP_CAPTURE_PATH  		ENABLED
#define DEFAULT_I2C_ADDR    		0x0

#define OMAP35XX_MASTER_CLK 		12288000L
#define OMAP35XX_PCM_RATES  		SND_PCM_RATE_48000
#define OMAP35XX_FRAME_RATE 		48000
#define OMAP35XX_TX_VOICES  		2
#define OMAP35XX_RX_VOICES  		2
#define OMAP35XX_CLK_MODE   		SLAVE

__SRCVERSION( "$URL:$ $Rev:$" )

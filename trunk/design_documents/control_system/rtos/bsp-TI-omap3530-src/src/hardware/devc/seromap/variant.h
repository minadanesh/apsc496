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






#ifdef OMAP5910

#ifndef write_omap
#define	write_omap(__port,__val)	out32(__port,__val)
#endif

#ifndef read_omap
#define	read_omap(__port)	in32(__port)
#endif

#else

#ifndef write_omap
#define	write_omap(__port,__val)	out8(__port,__val)
#endif

#ifndef read_omap
#define	read_omap(__port)	in8(__port)
#endif


#endif


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devc/seromap/variant.h $ $Rev: 249398 $" )

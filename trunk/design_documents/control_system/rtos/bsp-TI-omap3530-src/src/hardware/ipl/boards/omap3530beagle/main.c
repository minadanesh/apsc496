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
#include <hw/inout.h>
#include <arm/omap3530.h>
#include "nand.h"

extern int boot_from_flash;
extern void init_omap3530beagle(void);

#define FLASH_IMAGE_ADDR    0x08004000
#define	QNX_LOAD_ADDRESS	0x84000000
#define NAND_RESERVED_BLOCK	4
#define NAND_BLK_PER_IFS	256

#define NAND_RESERVED_BLOCK_START   4
#define NAND_RESERVED_BLOCK_END     255

int main(void)
{
	unsigned image;
	init_omap3530beagle();

	ser_putstr((char *)"\n\nQNX Neutrino Initial Program Loader for Texas Instruments OMAP3530 Beagle Board\n");

	while (1) {
		image = QNX_LOAD_ADDRESS;
		ser_putstr((char *)"Commands:\n\n");
		ser_putstr((char *)"Press 'D' for serial download, using the 'sendnto' utility\n");
		ser_putstr((char *)"Press 'F' to boot an OS image from NAND flash\n");
		ser_putstr((char *)"Press 'U' to Copy an OS image to NAND flash\n");
		ser_putstr((char *)"Press 'I' to Update the IPL\n");

		switch (ser_getchar()) {
			case 'D': case 'd':
				ser_putstr((char *)"Send image now...\n");
				if (image_download_ser(image)) {
					ser_putstr((char *)"Download failed...\n");
					continue;
				} 
				else
					ser_putstr((char *)"Download OK...\n");
				image = image_scan(image, image + 0x200);
				break;
			case 'F': case 'f':
					ser_putstr((char *)"reading from NAND flash ........\n");
					 if (read_image_from_nand(NAND_RESERVED_BLOCK_START, NAND_RESERVED_BLOCK_END, (unsigned)image) !=0 ) {
    					ser_putstr((char *)"Read from NAND flash failed...\n");
						continue;
					}
					image = image_scan(image, image+0x200);
					break;
			case 'U': case 'u':
					ser_putstr((char *)"send the ifs now...\n");
					if (image_download_ser(image)) {
						ser_putstr((char *)"download failed...\n");
						continue;
					}else{
						ser_putstr((char *)"download OK...\n");
						//read all left bytes
						while(ser_poll()){
							ser_getchar();
						}
		     			ser_putstr((char *)"Writing IFS image to NAND flash from block 4 ......\n");
                        if (upgrade_nand_flash(NAND_RESERVED_BLOCK_START, NAND_RESERVED_BLOCK_END, (unsigned)image) != 0 ) {
                            ser_putstr((char *)"IFS image upgrading failed...\n");
                        } else {
                            ser_putstr((char *)"Upgrade IFS OK...\n"); 
                        }
                        
						continue;
					}
						
					break;
		case 'I': case 'i':
                    ser_putstr((char *)"Send the IPL image, using the 'sendnto' utility...\n");
                    if (image_download_ser(image)) {
                        ser_putstr((char *)"Download failed...\n");
                        continue;
                    } else {
                        ser_putstr((char *)"Download OK...\n");
                        
                        ser_putstr((char *)"Writing IPL to NAND flash @ blk0 ......\n");
                        if (upgrade_IPL((unsigned)image) != 0 ) {
                            ser_putstr((char *)"IPL upgrading failed...\n");
                        }
                        else {
                            ser_putstr((char *)"Update IPL OK\n"); 
                        }

                        //read all left bytes
                        while (ser_poll()) {
                            ser_getchar();
                        }
                        continue;
                    }
                    break;
			default:
				continue;
		}

		if (image != 0xffffffff) {
			//before we go, we need to unlock the top 128MB NAND
			// this will automatically lock all other blocks
			nand_unlock_blocks(1024, 2047);
			ser_putstr((char *)"Found image               @ 0x");
			ser_puthex(image);
			ser_putstr((char *)"\n");
			image_setup(image);
			ser_putstr((char *)"Jumping to startup        @ 0x");
			ser_puthex(startup_hdr.startup_vaddr);
			ser_putstr((char *)"\n\n");
			image_start(image);

			/* Never reach here */
			return 0;
		}else{
			ser_putstr((char *)"image_scan failed...\n");
		}
	}

	return 0;
}


__SRCVERSION( "$URL$ $Rev$" );



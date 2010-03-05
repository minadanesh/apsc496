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


#include <arm/omap2420.h>
#include <arm/omap3530.h>
#include "nand.h"


#define MAX_IMAGE_SIZE  0x02000000      //32 MB


unsigned nand_wait_busy(NAND_CHIP *dev, uint32_t usec) {
    uint32_t status;
    status = OMAP_READ_GPMC_STATUS();
    while (!(status & OMAP_GPMC_WAITPIN0)) {
        status = OMAP_READ_GPMC_STATUS();
        if (usec-- == 0) {
            return (-1);
        }
    }
    return 0;
}

// Accessing the command reg automatically sets ALE=0, CLE=1
void nand_write_cmd(NAND_CHIP *dev, int command) {
    OMAP_WRITE_NAND_COMMAND(command);
}

// Accessing the address reg automatically sets ALE=1, CLE=0
void nand_write_pageaddr(NAND_CHIP *dev, unsigned page, int addr_cycles) {
    int i;
    unsigned short  addr[5];

    addr[0] = addr[1] = 0x0 ;
    addr[2] = NAND_ADDR_ROW1(page);
    addr[3] = NAND_ADDR_ROW2(page);
    addr[4] = NAND_ADDR_ROW3(page);

    for (i = 0; i < addr_cycles; i++) {
        OMAP_WRITE_NAND_ADDRESS(addr[i]);
    }
}

// Accessing the address reg automatically sets ALE=1, CLE=0
void nand_write_blkaddr(NAND_CHIP *dev, unsigned blk, int addr_cycles) {
    int i;
    unsigned short  addr[5];
    unsigned        page = blk * PAGES2BLK_2048;

    addr[0] = NAND_ADDR_ROW1(page);
    addr[1] = NAND_ADDR_ROW2(page);
    addr[2] = NAND_ADDR_ROW3(page);
    
    for (i = 0; i < addr_cycles; i++) {
        OMAP_WRITE_NAND_ADDRESS(addr[i]);
    }
}

// Accessing the data reg automatically sets ALE=0, CLE=0
void nand_write_data(NAND_CHIP *dev, uint8_t *databuffer, int data_cycles) {
    int i;
    uint16_t    *p1 = (uint16_t *)databuffer;

    for (i = 0; i < data_cycles; i += 2) {
        OMAP_WRITE_NAND_DATA(*p1++);
    }
}

// Accessing the data reg automatically sets ALE=0,CLE=0
void nand_read_data(NAND_CHIP *dev, uint8_t *databuffer, int data_cycles) {
    int i;
    uint16_t    *p1 = (uint16_t *)databuffer;

    for (i = 0; i < data_cycles; i += 2) {
        *p1++ = OMAP_READ_NAND_DATA();
    }
}

// Accessing the data reg automatically sets ALE=0,CLE=0
void nand_read_status(NAND_CHIP *dev, uint8_t *databuffer, int data_cycles) {
    int i; 

    for (i = 0; i < data_cycles; i++) {
        databuffer[i]= OMAP_READ_NAND_DATA();
    }
}

unsigned nand_init() {
    uint8_t id[5], status=0;

    dev = &chip_dev;
    dev->iobase = OMAP3530_GPMC_BASE;

    //Init hwecc
    *OMAP_GPMC_ECC_CONTROL = 0x101;             // Clear all ECC result registers, ECC result register 1 selected
    *OMAP_GPMC_ECC_SIZE_CONFIG = 0x3fcff000;    // Default value, 512 Bytes 

    //Init prefech engine
    GPMC_PREFETCH_CONTROL = 0;                  //stop the engine for now
    GPMC_PREFETCH_CONFIG1 = 0x0;
    GPMC_PREFETCH_CONFIG2 = 0x0;

    // Reset the part
    nand_write_cmd(dev, NANDCMD_RESET_2048);
    if (nand_wait_busy(dev, MAX_RESET_USEC) != 0) {
        ser_putstr("NAND init: Timeout on RESET\n");
        return (-1);
    }

    nand_write_cmd(dev, NANDCMD_STATUSREAD_2048);
    nand_read_status(dev, &status, 1);

    // Read id info from the part
    nand_write_cmd(dev, NANDCMD_IDREAD_2048);
    nand_write_pageaddr(dev, 0, 1);             // Address, 1 cycle 
    if (nand_wait_busy(dev, MAX_READ_USEC) != 0) {
        ser_putstr((char *)"NAND init: Timeout on ReadID");
        return (-1);
    }
    
    nand_read_status(dev, id, 5);               // Get Device ID and Configuration Codes 
 
    switch (id[1]) {
        // 256M
        case 0xaa: case 0xda: case 0xba: case 0xca:
            dev->numblks = 2048;
            dev->addrcycles = 5;
            break;
        
        // 512M, (MT29F4G16ABCHC on OMAP3517 EVM)
        case 0xbc:
            dev->numblks = 4096;
            dev->addrcycles = 5;
            break;

        default:
            ser_putstr((char *)"nand_init: Unsupported NAND chip\n");
            return (-1);
            break;
    }

    // We glue to physical pages at the driver and report back their combined size
    dev->sparesize  = SPARESIZE_2048;
    dev->blksize    = (DATASIZE_2048 + dev->sparesize) * PAGES2BLK_2048;
 
    return 0;
}

 int nand_read_page(NAND_CHIP *dev, unsigned page, uint8_t *databuffer, int data_cycles, int op, uint8_t *ecc)
 {
    nand_write_cmd(dev, NANDCMD_READ_2048);
    if (!op) {
        nand_write_pageaddr(dev, page, dev->addrcycles);
    } else {
        int i;
        unsigned short  addr[5];
        addr[0] = 0x80;
        addr[1] = DATASIZE_2048>>9 ;
        addr[2] = NAND_ADDR_ROW1(page);
        addr[3] = NAND_ADDR_ROW2(page);
        addr[4] = NAND_ADDR_ROW3(page);
        for (i = 0; i < 5; i++) {
            OMAP_WRITE_NAND_ADDRESS(addr[i]);
        }
    }
    nand_write_cmd(dev, NANDCMD_READCONFIRM_2048);

    if (nand_wait_busy(dev,  MAX_READ_USEC) != 0) {
        ser_putstr((char *)"Timeout on READ");
    }

    if (ecc) {
        //reset ecc
        *OMAP_GPMC_ECC_CONTROL = 0x101;
        *OMAP_GPMC_ECC_CONFIG = 0x81; 
    }

    nand_read_data(dev, databuffer, data_cycles);

    if (ecc) {
        uint32_t    val;
        int         i;
        for (i=0; i<4; i++) {
            val = GPMC_ECC_RESULT(i);
            *ecc++ = ECC_P1_128_E(val);
            *ecc++ = ECC_P1_128_O(val);
            *ecc++ = ECC_P512_2048_E(val) | ECC_P512_2048_O(val) << 4;
        }
    }
 
    return data_cycles;
}
 
int nand_write_page(NAND_CHIP *dev, unsigned page, uint8_t *databuffer, uint8_t *sparebuffer)
{
    uint8_t     status;
    uint8_t     *ecc_code;
    uint32_t    val;
    int         i;

    ecc_code = sparebuffer+2; 
    nand_write_cmd(dev, NANDCMD_PROGRAM_2048);
    nand_write_pageaddr(dev, page, dev->addrcycles);

    //reset ecc
    *OMAP_GPMC_ECC_CONTROL = 0x101;
    *OMAP_GPMC_ECC_CONFIG = 0x81; 
    nand_write_data(dev, databuffer, DATASIZE_2048);

    for(i=0; i<4; i++){
        val = GPMC_ECC_RESULT(i);
        *ecc_code++ = ECC_P1_128_E(val);
        *ecc_code++ = ECC_P1_128_O(val);
        *ecc_code++ = ECC_P512_2048_E(val) | ECC_P512_2048_O(val) << 4;
    }

    nand_write_data(dev, sparebuffer, SPARESIZE_2048);
    nand_write_cmd(dev, NANDCMD_PROGRAMCONFIRM_2048);
     
    if(nand_wait_busy(dev, MAX_POST_USEC) != 0)
        ser_putstr((char *)"Timeout on POST");
     
    nand_write_cmd(dev, NANDCMD_STATUSREAD_2048);        // Read status
    nand_read_status(dev, &status, 1);
    if((status & 0xC3) != 0xC0) {
        ser_putstr((char *)"Post error on page");
        ser_puthex(page);
        ser_putstr((char *)"\n");
        return(-1);
    }

    return 0;
}
 
int nand_erase_blk(NAND_CHIP *dev, unsigned blk)
{
    uint8_t status;
    
    nand_write_cmd(dev, NANDCMD_ERASE_2048);
    nand_write_blkaddr(dev, blk, 3);    
    nand_write_cmd(dev, NANDCMD_ERASECONFIRM_2048);
    if (nand_wait_busy(dev, MAX_ERASE_USEC) != 0) {
        ser_putstr((char *)"Timeout on ERASE Block");
        return (-1);
    }
    
    nand_write_cmd(dev, NANDCMD_STATUSREAD_2048);
    nand_read_status(dev, &status, 1);  
    if ((status & 0xC3) != 0xC0) {
        ser_putstr((char *)"Erase error on blk ");
        ser_puthex(blk);
        ser_putstr((char *)" status ");
        ser_puthex(status);
        ser_putstr((char *)"\n");
        return (-1);
    }

    return 0;
}
 
 
unsigned GetBlockStatus(NAND_CHIP *dev, unsigned blockID)
{
    PageInfo    PI;
    unsigned    page = PAGES2BLK_2048 * blockID;
    
    nand_read_page(dev, page, (uint8_t *)&PI, SPARESIZE_2048, 1, 0);
    if (PI.status != NAND_BLK_VALID) {
        return 1;
    } else {
        return 0;
    }
}

unsigned nand_unlock_blocks(unsigned start, unsigned end)
{
    nand_write_cmd(dev, NANDCMD_UNLOCK_LOW);
    nand_write_blkaddr(dev, start, 3);
    nand_write_cmd(dev, NANDCMD_UNLOCK_HIGH);
    nand_write_blkaddr(dev, end, 3);  
    if (nand_wait_busy(dev, MAX_ERASE_USEC) != 0) {
        ser_putstr((char *)"Timeout on Block unlock");
    }
     
    return 0;
}

unsigned upgrade_nand_flash(unsigned start_block, unsigned end_block, unsigned src)
{
    PageInfo    PI;
    int         imagesize=0;
    unsigned    page;
    unsigned    status;
    int         sequence=0, i;
    struct      startup_header *hdr;

    //scan the image to get the correct size
    for (i=0; i<DATASIZE_2048; i+=4) {
        hdr = (struct startup_header *)(src+i);
        if (hdr->signature == STARTUP_HDR_SIGNATURE) {
            imagesize = hdr->stored_size;
            break;
        }
    }

    if (imagesize==0) {
        ser_putstr((char *)"Invalid image, can not find the signature\n");
        return (-1);        
    }
    
    /* First we need unlock the blocks we need*/
    nand_unlock_blocks(start_block, end_block);

    PI.status = NAND_BLK_VALID;
    for (; start_block <= end_block; start_block++) {
        //is this a good block, otherwise skip this block
        status=GetBlockStatus(dev, start_block);

        if (status == 0) {
            //then erase this block
            if (nand_erase_blk(dev, start_block) == -1) {
                ser_putstr((char *)"NAND erase failed\n");
                return (-1);
            }

            //write data to this block
            page = start_block * PAGES2BLK_2048;
            for ( ; page < ((start_block+1) * PAGES2BLK_2048); page++) {
                PI.sequence   = sequence;
                if (nand_write_page(dev, page, (uint8_t *)src, (uint8_t *)&PI) == -1) {
                    ser_putstr((char *)"NAND post failed on page: \n");
                    ser_puthex(page);
                    ser_putstr((char *)"\n");
                    return (-1);
                }
                imagesize -= DATASIZE_2048;
                if (imagesize <= 0) {
                    return 0;
                }
                src += DATASIZE_2048;
                sequence++;
            }
        } else {
            ser_putstr((char *)"Bad Block: blk ");
            ser_puthex(start_block);
            ser_putstr((char *)"\n");
        }
    }
    return (-1);
}

unsigned upgrade_IPL(unsigned src)
{
    PageInfo    PI;
    int         imagesize = 0; 
    int         blk_id = 0;
    unsigned    page;
    int         i;
    
    PI.status = NAND_BLK_VALID;

    // the first 4 bytes is the image size
    imagesize = *(uint32_t *)(src);
    if ((imagesize <= 0) || (imagesize > 512*1024)) {
        ser_putstr((char *)"Invalid IPL image size = ");
        ser_puthex(imagesize);
        ser_putstr((char *)"\n");
        return (-1);
    }

    //unlock the first 4 block
    nand_unlock_blocks(blk_id, blk_id+3);
    
    // erase the first 4 blocks
    for (i = 0; i < 4; i++) {
        if (nand_erase_blk(dev, blk_id+i) == -1) {
            ser_putstr((char *)"NAND erase failed on blk: \n");
            ser_puthex(blk_id+i);
            ser_putstr((char *)"\n");
            return (-1);
        }

        //write data to the block, assume IPL is not bigger than 4 blocks
        page = blk_id * PAGES2BLK_2048;
        for ( ; page < ((blk_id + 1) * PAGES2BLK_2048); page++) {
            if (nand_write_page(dev, page, (uint8_t *)src, (uint8_t *)&PI)==-1) {
                ser_putstr((char *)"NAND post failed on page: \n");
                ser_puthex(page);
                ser_putstr((char *)"\n");
                return (-1);
            }
            imagesize -= DATASIZE_2048;
            if (imagesize <= 0)
                return 0;
            src+=DATASIZE_2048;
        }
    }
    
    if (imagesize > 0) {
        ser_putstr((char *)"Invalid IPL image size\n");
        return (-1);
    }
    return 0;
}
 
unsigned read_image_from_nand(unsigned start_block, unsigned end_block, unsigned dst)
{
    PageInfo    *PI;
    int         sequence = 0;
    int         i;
    unsigned    page;
    unsigned    status = 0;
    int         imagesize = 0;
    uint8_t     ecc[12];
    struct      startup_header *hdr;

    for (; start_block<=end_block; start_block++) {
        //first, is this a good block
        status = GetBlockStatus(dev, start_block);
        if (status == 0) {
            //read data from this block
            page = start_block*PAGES2BLK_2048;
            for( ; page < ((start_block + 1) * PAGES2BLK_2048); page++) {

                if (nand_read_page(dev, page, (uint8_t *)dst, PAGESIZE_2048, 0, ecc) == -1) {
                    return (-1);
                }

                PI = (PageInfo *)(dst + DATASIZE_2048);
                if (PI->status != NAND_BLK_VALID) { 
                    ser_putstr((char *)"Bad blk, page ");
                    ser_puthex(page);
                    ser_putstr((char *)" status ");
                    ser_puthex(PI->status);
                    ser_putstr((char *)" \n");
                    continue ;
                }

                for (i = 0; i < 12; i++) {
                    if (ecc[i] != PI->ecc[i]) {
                        ser_putstr((char *)"Incorrect ECC, page ");
                        ser_puthex(page);
                        ser_putstr((char *)"\n");
                        ser_puthex(ecc[i]);
                        ser_putstr((char *)" ");
                        ser_puthex(PI->ecc[i]);
                        ser_putstr((char *)"\n");
                        return (-1);
                    }
                }

                if (PI->sequence != sequence) {
                    ser_putstr((char *)"Incorrect sequence number, page ");
                    ser_puthex(page);
                    ser_putstr((char *)"\n");
                    ser_puthex(sequence);
                    ser_putstr((char *)" ");
                    ser_puthex(PI->sequence);
                    ser_putstr((char *)"\n");
                    return (-1);
                }

                if (imagesize == 0) {
                    for (i = 0; i < DATASIZE_2048; i+=4) {
                        hdr = (struct startup_header *)(dst+i);
                        if (hdr->signature == STARTUP_HDR_SIGNATURE) {
                            imagesize = hdr->stored_size;
                            break;
                        }
                    }
                    
                    if ((imagesize <= 0) || (imagesize > MAX_IMAGE_SIZE)) { 
                        ser_putstr((char *)"In correct image size page ");
                        ser_puthex(page);
                        ser_putstr((char *)"\n");
                        ser_puthex(imagesize);
                        ser_putstr((char *)" ");
                        ser_puthex(hdr->stored_size);
                        ser_putstr((char *)"\n");
                        return (-1);
                    }
                }
                    
                imagesize -= DATASIZE_2048;
                if (imagesize <= 0) {
                    return 0;
                }
                dst+=DATASIZE_2048;
                sequence++;
            }
        }
    }   

    return (-1);
}



__SRCVERSION( "$URL$ $Rev$" );

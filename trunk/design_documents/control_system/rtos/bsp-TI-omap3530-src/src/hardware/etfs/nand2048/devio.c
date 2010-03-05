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


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <gulliver.h>
#include <sys/slog.h>
#include <sys/neutrino.h>
#include <fs/etfs.h>
#include "devio.h"


//
// Initialize the part and stuff physical paramaters for the part.
//
int devio_init(struct etfs_devio *dev) {
	uint8_t			id[4];
	CHIPIO			*cio = dev->cio;

	// Allow IO operations
	if(ThreadCtl(_NTO_TCTL_IO, 0) != EOK) {
		dev->log(_SLOG_CRITICAL, "You must be root.");
		// We will not return
	}
	
	// Do anything special to get the part ready to use
	if(nand_init(dev) != 0) {
		dev->log(_SLOG_CRITICAL, "nand_init failed : %s", strerror(errno));
		// We will not return
	}

	// Reset the part
	nand_write_cmd(cio, NANDCMD_RESET);
	if(nand_wait_busy(cio, MAX_RESET_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on RESET");

	// Read id info from the part
	nand_write_cmd(cio, NANDCMD_IDREAD);
	nand_write_pageaddr(cio, 0, 2);
	nand_read_data(cio, id, 4);

	switch(id[1]) {
	// A Fake 32M part for testing
	case 0x00:
		dev->numblks = 256;
		cio->addrcycles = 4;
		break;

	// 128M
	case 0xa1: case 0xf1: case 0xb1: case 0xc1:
		dev->numblks = 1024;
		cio->addrcycles = 4;
		break;

	// 256M
	case 0xaa: case 0xda: case 0xba: case 0xca:
		dev->numblks = 2048;
		cio->addrcycles = 4;
		break;

	// 512M
	case 0xac: case 0xdc: case 0xbc: case 0xcc:
		dev->numblks = 4096;
		cio->addrcycles = 4;
		break;

	default:
		dev->log(_SLOG_CRITICAL, "Unsupported NAND device (%2.2x %2.2x)", id[0], id[1]);
		// We will not return
	}

	// These must be initialized here. All other numbers are built from them.
	sprintf(dev->id, "NAND%2.2x%2.2x", id[0], id[1]);
	dev->memtype       = ETFS_MEMTYPE_NAND;
	dev->clustersize   = DATASIZE;
	dev->sparesize     = SPARESIZE;
	dev->clusters2blk  = PAGES2BLK;
	dev->blksize       = (dev->clustersize + SPARESIZE) * dev->clusters2blk;

	cio->lastpage = ~0;

	return(EOK);
}


//
// Read a cluster of data.
//
// Verify crc for both the spare area and the entire page (data + spare).
// The passed buffer "buf" is larger than the cluster size. It can hold
// (PAGESIZE bytes. This is for convienence when reading
// data from the device ands calculating the crc. The work area after clustersize
// bytes is ignored by the caller.
//
int devio_readcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp) {
	struct spare		*sp;
	int					err;
	unsigned			page = cluster;
	CHIPIO				*cio = dev->cio;

	//
	// Read page from the device
	//
	if(cio->lastcmd != NANDCMD_READ || (page & (PAGES2BLK-1)) == 0 || cio->lastpage + 1 != page) {
		nand_write_cmd(cio, NANDCMD_READ);
		cio->inspare = 0;
		nand_write_pageaddr(cio, page, cio->addrcycles);
		}

	if(nand_wait_busy(cio,  MAX_READ_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on READ");
	nand_read_data(cio, buf, PAGESIZE);
	cio->lastpage = page;

	sp = (struct spare *)(buf + DATASIZE);
	if(sp->status != 0xff || sp->status2 != 0xff) {
		dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
		trp->tacode = ETFS_TRANS_BADBLK;

	} else if(((uint64_t *)sp)[2] == ~0ll  &&  ((uint64_t *)sp)[3] == ~0ll
 	       && ((uint64_t *)sp)[4] == ~0ll  &&  ((uint64_t *)sp)[5] == ~0ll
 	       && ((uint64_t *)sp)[6] == ~0ll  &&  ((uint64_t *)sp)[7] == ~0ll)
		if(sp->erasesig[0] == ERASESIG1 && sp->erasesig[1] == ERASESIG2)
			trp->tacode = ETFS_TRANS_ERASED;
		else
			trp->tacode = ETFS_TRANS_FOXES;

	else if(dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans)) != sp->crctrans) {
		dev->log(_SLOG_ERROR, "readcluster trans DATAERR on cluster %d", cluster);
		trp->tacode = ETFS_TRANS_DATAERR;

	} else
		trp->tacode = ETFS_TRANS_OK;

	// Build transaction data from data in the spare area.
	trp->sequence    = ENDIAN_LE32(sp->sequence);
	trp->fid         = ENDIAN_LE16(sp->fid);
	trp->cluster     = ENDIAN_LE16(sp->clusterlo) + (sp->clusterhi << 16);
	trp->nclusters   = sp->nclusters;

	trp->dacode = ETFS_TRANS_OK;
	if(trp->tacode == ETFS_TRANS_OK) {
		if(dev->crc32(buf, DATASIZE) != sp->crcdata) {
			err = dev->ecc(buf, &sp->eccv[0], 8, 1);
			if(err >= ETFS_ECC_ERROR || dev->crc32(buf, DATASIZE) != sp->crcdata) {
				dev->log(_SLOG_ERROR, "readcluster DATAERR on cluster %d", cluster);
				return(trp->dacode = ETFS_TRANS_DATAERR);
			}

			dev->log(_SLOG_ERROR, "readcluster ECCERR on cluster %d", cluster);
			return(trp->dacode = ETFS_TRANS_ECC);
		}
	}

	return(trp->tacode);
}


//
// Read the spare area of a page (not the data) to return transaction infoormation.
//
// This called is used heavily on startup to process the transactions. It is
// a cheaper call than readcluster() since it reads less data and has a smaller
// checksum to calculate.
//
int devio_readtrans(struct etfs_devio *dev, unsigned cluster, struct etfs_trans *trp) {
	struct spare		spare;
	unsigned			page = cluster;
	CHIPIO				*cio = dev->cio;

	// We pipeline reads to the spare area since we may
	// read it sequentially as a scan on startup.
	if(cio->lastcmd != NANDCMD_SPAREREAD || (page & (PAGES2BLK-1)) == 0 || cio->lastpage + 1 != page) {
		nand_write_cmd(cio, NANDCMD_SPAREREAD);
		cio->inspare = 1;
		nand_write_pageaddr(cio, page, cio->addrcycles);
	}

	if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on READ");
	nand_read_data(cio, (uint8_t *)&spare, sizeof(spare));
	cio->lastpage = page;

	if(spare.status != 0xff || spare.status2 != 0xff) {
		dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
		return(ETFS_TRANS_BADBLK);
	}

	if(((uint64_t *)&spare)[2] == ~0ll  &&  ((uint64_t *)&spare)[3] == ~0ll
    && ((uint64_t *)&spare)[4] == ~0ll  &&  ((uint64_t *)&spare)[5] == ~0ll
    && ((uint64_t *)&spare)[6] == ~0ll  &&  ((uint64_t *)&spare)[7] == ~0ll)
		if(spare.erasesig[0] == ERASESIG1 && spare.erasesig[1] == ERASESIG2)
			return(ETFS_TRANS_ERASED);
		else
			return(ETFS_TRANS_FOXES);

	if(dev->crc32((uint8_t *) &spare, sizeof(spare) - sizeof(spare.crctrans)) != spare.crctrans) {
		dev->log(_SLOG_ERROR, "readtrans DATAERR on cluster %d", cluster);
		return(ETFS_TRANS_DATAERR);
	}

	trp->sequence    = ENDIAN_LE32(spare.sequence);
	trp->fid         = ENDIAN_LE16(spare.fid);
	trp->cluster     = ENDIAN_LE16(spare.clusterlo) + (spare.clusterhi << 16);
	trp->nclusters   = spare.nclusters;

	return(ETFS_TRANS_OK);
}


//
// Post a cluster of data.
//
// Set crc for both the spare area and the entire page (data + spare).
// The passed buffer "buf" is larger than the cluster size. It can hold
// PAGESIZE bytes. This is for convienence writing
// data to the device ands calculating the crc. The work area after clustersize
// bytes is ignored by the caller.
//
int devio_postcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp) {
	struct spare		*sp;
	uint8_t				status;
	unsigned			page = cluster;
	CHIPIO				*cio = dev->cio;

	// Build spare area
	sp = (struct spare *) (buf + DATASIZE);
	memset((void *)sp, 0xff, sizeof(*sp));
	sp->erasesig[0] = ERASESIG1;
	sp->erasesig[1] = ERASESIG2;
	if(trp) {
		sp->sequence   = ENDIAN_LE32(trp->sequence);
		sp->fid        = ENDIAN_LE16((uint16_t) trp->fid);
		sp->clusterlo  = ENDIAN_LE16((uint16_t) trp->cluster);
		sp->clusterhi  = trp->cluster >> 16;
		sp->nclusters  = trp->nclusters;
		sp->status     = 0xff;
		sp->status2    = 0xff;
		sp->crcdata    = dev->crc32(buf, DATASIZE);
		dev->ecc(buf, &sp->eccv[0], 8, 0);
		sp->crctrans   = dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans));
		if(cluster % dev->clusters2blk == 0) {
			sp->erasesig[0] = ~0;	// Can only punch bits down once and we did it
			sp->erasesig[1] = ~0;	// on the erase
		}
	}

	// Write page of data
	// This forces us out of the spare area.
	if(cio->inspare) {
		nand_write_cmd(cio, NANDCMD_READ);
		cio->inspare = 0;
	}
	nand_write_cmd(cio, NANDCMD_PROGRAM);
	nand_write_pageaddr(cio, page, cio->addrcycles);
	nand_write_data(cio, buf, PAGESIZE);
	nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM);

	if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on POST");

	nand_write_cmd(cio, NANDCMD_STATUSREAD);
	nand_read_data(cio, &status, 1);

	if((status & 0x01) != 0) {
		dev->log(_SLOG_ERROR, "Post error on page %d", page);
		return(ETFS_TRANS_DEVERR);
	}

	return(ETFS_TRANS_OK);
}


//
// Erase a block.
//
int devio_eraseblk(struct etfs_devio *dev, unsigned blk) {
	uint8_t			status;
	CHIPIO			*cio = dev->cio;
	uint8_t			*buf = alloca(PAGESIZE);

	// Erase the block
	nand_write_cmd(cio, NANDCMD_ERASE);
	nand_write_blkaddr(cio, blk, cio->addrcycles);
	nand_write_cmd(cio, NANDCMD_ERASECONFIRM);
	if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on ERASE");
	nand_write_cmd(cio, NANDCMD_STATUSREAD);
	nand_read_data(cio, &status, 1);

	if(status != 0xC0) {
		dev->log(_SLOG_ERROR, "erase error on blk %d (%2.2x)", blk, status);
		return(ETFS_TRANS_DEVERR);
	}

	// Write the erase signature. We only write non FFs in the first 16 bytes of the
	// spare area and put FFs everywhere else. This is required for
	// multilevel NAND devices.
	memset(buf, 0xff, PAGESIZE);
	status = devio_postcluster(dev, blk * dev->clusters2blk, buf, NULL);
	return(status);
}


//
// Called to allow the driver to flush any cached data that
// has not be written to the device. The NAND class driver does
// not need it.
//
int devio_sync(struct etfs_devio *dev) {

	return(-1);
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/etfs/nand2048/devio.c $ $Rev: 249398 $" );

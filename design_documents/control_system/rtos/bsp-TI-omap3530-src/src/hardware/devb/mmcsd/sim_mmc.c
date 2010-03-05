/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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

// Module Description:  MMC command processing

#include <sim_mmc.h>

static void mmc_media_check(SIM_HBA *hba);

ssize_t mmc_slogf(SIM_HBA *hba, int opcode, int severity, int vlevel, const char *fmt, ...)
{
	ssize_t		ret;
	va_list		arglist;
	int			verbosity;

	ret			= 0;
	verbosity	= hba ? hba->verbosity : mmc_ctrl.verbosity;
	
	if (verbosity > 5) {
		va_start(arglist, fmt);
		vfprintf(stderr, fmt, arglist);
		va_end(arglist);
		fprintf(stderr, "\n");
	}
	if (vlevel <= 4 || verbosity >= vlevel) {
		va_start(arglist, fmt);
		ret = vslogf(opcode, severity, fmt, arglist);
		va_end(arglist);
	}
	return (ret);
}

static void mmc_sense(SIM_HBA *hba, int sense, int asc, int ascq)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;
	SCSI_SENSE	*sptr;

	if (ccb) {
		ccb->cam_scsi_status	= SCS_CHECK;
		ccb->cam_ch.cam_status	= CAM_REQ_CMP_ERR;
		sptr					= (SCSI_SENSE *)ccb->cam_sense_ptr;

		if ((ccb->cam_ch.cam_flags & CAM_DIS_AUTOSENSE) || sptr == NULL)
			return;

		ccb->cam_ch.cam_status |= CAM_AUTOSNS_VALID;

		memset(sptr, 0, sizeof(*sptr));
		sptr->error	= 0x70;		// Error code
		sptr->sense = sense;	// Sense key
		sptr->asc	= asc;		// Additional sense code (Invalid field in CDB)
		sptr->ascq	= ascq;		// Additional sense code qualifier
	}
}

static void mmc_reset(SIM_HBA *hba)
{
	// TODO
}

static void mmc_error(SIM_HBA *hba, int mmc_status)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;

	if (ccb) {
		switch (mmc_status) {
			case MMC_DATA_OVERRUN:
				ccb->cam_ch.cam_status = CAM_DATA_RUN_ERR;
				break;
			case MMC_NOT_PRESENT:
				mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
				break;
			case MMC_TIMEOUT:
			case MMC_COMMAND_FAILURE:
				mmc_reset(hba);
				ccb->cam_ch.cam_status = CAM_CMD_TIMEOUT;
				break;
			case MMC_READ_ERROR:			// CRC errors
			case MMC_WRITE_ERROR:
			default:
				ccb->cam_ch.cam_status = CAM_CMD_TIMEOUT;
				break;
		}
	}
}

static void mmc_msense(SIM_HBA *hba)
{
	SIM_MMC_EXT			*ext;
	CCB_SCSIIO			*ccb;
	MODE_PARM_HEADER10	*hdr;

	ext = (SIM_MMC_EXT *)hba->ext;
	ccb = ext->nexus;

	if (!(ext->eflags & MMC_EFLAG_READY)) {
		mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
		return;
	}

	switch (ccb->cam_cdb_io.cam_cdb_bytes[2]) {
		case MP_RW_ERR:
			hdr = (MODE_PARM_HEADER10 *)ccb->cam_data.cam_data_ptr;
			memset(hdr, 0, sizeof(*hdr));
			if (ext->eflags & MMC_EFLAG_WP)
				hdr->device_specific |= MP_DS_WP;

			ccb->cam_ch.cam_status = CAM_REQ_CMP;
			ccb->cam_scsi_status   = SCS_GOOD;
			break;
		default:
			mmc_sense(hba, SK_ILLEGAL, ASC_INVALID_FIELD, 0);
			break;
	}
}

static void mmc_unit_ready(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;

	if (ext->eflags & MMC_EFLAG_MEDIA_CHANGED) {
		mmc_sense(hba, SK_UNIT_ATN, ASC_MEDIUM_CHANGED, ASCQ_UNKNOWN_CHANGED);
		atomic_clr(&ext->eflags, MMC_EFLAG_MEDIA_CHANGED);
	} else if (ext->eflags & MMC_EFLAG_READY) {
		if (ext->detect(hba) != MMC_SUCCESS) {
			mmc_media_check(hba);
			mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
		} else
			ccb->cam_ch.cam_status = CAM_REQ_CMP;
	} else
		mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
}


static void mmc_inquiry(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	CCB_SCSIIO		*ccb;
	SCSI_INQUIRY	*iptr;
	char			buf[8];

	ext		= (SIM_MMC_EXT *)hba->ext;
	ccb		= ext->nexus;
	iptr	= (SCSI_INQUIRY *)ccb->cam_data.cam_data_ptr;

	if (ccb->cam_ch.cam_flags & CAM_DATA_PHYS) {
		mmc_error(hba, CAM_PROVIDE_FAIL);
		return;
	}

	if (ext->state == MMCSD_STATE_IDENT)
		return;

	memset(iptr, 0, sizeof(*iptr));

	iptr->peripheral	= D_DIR_ACC | INQ_QUAL_AVAIL;
	iptr->rmb			= CAM_TRUE;		// removable
	iptr->version		= 1;
	iptr->adlen			= 32;

	if (!(ext->eflags & MMC_EFLAG_READY)) {
		mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
		return;
	}

	if (ext->version < MMC_VERSION_1) {
		/* Vendor ID */
		strcpy((char *)&iptr->vend_id[0], "SD:");
		ultoa(ext->cid.sd_cid.mid, buf, 10);
		iptr->vend_id[3] = buf[0];
		iptr->vend_id[4] = buf[1];
		iptr->vend_id[5] = buf[2];

		/* Product ID */
		strcpy((char *)&iptr->prod_id[0], (char *)ext->cid.sd_cid.pnm);

		/* Product revision level, BCD code */
		iptr->prod_rev[0] = (ext->cid.sd_cid.prv >> 4) + '0';
		iptr->prod_rev[1] = '.';
		iptr->prod_rev[2] = (ext->cid.sd_cid.prv & 0x0F) + '0';
	} else {
		strcpy((char *)&iptr->vend_id[0], "MMC:");

		if (ext->csd.mmc_csd.mmc_prot < 2) {
			ultoa(ext->cid.mmc_cid.mid >> 16, buf, 10);

			/* Product revision level, BCD code */
			iptr->prod_rev[0] = ext->cid.mmc_cid.hwr + '0';
			iptr->prod_rev[1] = '.';
			iptr->prod_rev[2] = ext->cid.mmc_cid.fwr + '0';
		} else {
			ultoa(ext->cid.mmc_cid.mid, buf, 10);
		}
		iptr->vend_id[4] = buf[0];
		iptr->vend_id[5] = buf[1];
		iptr->vend_id[6] = buf[2];
		/* Product ID */
		strcpy((char *)&iptr->prod_id[0], (char *)ext->cid.mmc_cid.pnm);

		// Vendor Specific
		ultoa(ext->cid.mmc_cid.psn, (char *)&iptr->vend_spc[0], 16);
	}

	ccb->cam_ch.cam_status = CAM_REQ_CMP;
	ccb->cam_scsi_status   = SCS_GOOD;
}

// this routine is only needed if the driver reports a not ready condition
static void mmc_spindle(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;

	ext = (SIM_MMC_EXT *)hba->ext;
	ccb = ext->nexus;

	if (!(ext->eflags & MMC_EFLAG_READY)) {
		mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
		return;
	}

	switch (ccb->cam_cdb_io.cam_cdb_bytes[4]) {
		case MMC_SPINDLE_STOP:
		case MMC_SPINDLE_START:
			ccb->cam_ch.cam_status	= CAM_REQ_CMP;
			ccb->cam_scsi_status	= SCS_GOOD;
			break;
		case MMC_SPINDLE_EJECT:
		case MMC_SPINDLE_LOAD:
		default:
			mmc_sense(hba, SK_ILLEGAL, 0x24, 0);
			break;
	}
}

static void mmc_capacity(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	READ_CAPACITY	*cptr;
	CCB_SCSIIO		*ccb;
	uint32_t		csize, csizem, bsize, lba;

	ext		= (SIM_MMC_EXT *)hba->ext;
	ccb		= ext->nexus;
	cptr	= (READ_CAPACITY *)ccb->cam_data.cam_data_ptr;

	if (ccb->cam_ch.cam_flags & CAM_DATA_PHYS) {
		mmc_error(hba, CAM_PROVIDE_FAIL);
		return;
	}

	if (!(ext->eflags & MMC_EFLAG_READY)) {
		mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
		return;
	}

	if (ext->state == MMCSD_STATE_IDENT)
		return;

	if (ext->version < MMC_VERSION_1) {
		// SD
		if (ext->csd.sd_csd.csd_structure == 0) {
			bsize  = 1 << ext->csd.sd_csd.read_bl_len;
			csize  = ext->csd.sd_csd.csd.csd_ver1.c_size + 1;
			csizem = 1 << (ext->csd.sd_csd.csd.csd_ver1.c_size_mult + 2);
		} else {
			bsize  = MMC_DFLT_BLKSIZE;
			csize  = ext->csd.sd_csd.csd.csd_ver2.c_size + 1;
			csizem = 1024;
		}
	} else {
		if((ext->csd.mmc_csd.mmc_prot >=CSD_VERSION_14) && (ext->csd.mmc_csd.ext_csd.sectors)){
			bsize  = MMC_DFLT_BLKSIZE;
			csize  = ext->csd.mmc_csd.ext_csd.sectors;
			csizem = 1;
		} else {
			bsize  = 1 << ext->csd.mmc_csd.read_bl_len;
			csize  = ext->csd.mmc_csd.c_size + 1;
			csizem = 1 << (ext->csd.mmc_csd.c_size_mult + 2);
		}
	}

	// force to 512 byte block
	if (bsize > MMC_DFLT_BLKSIZE && (bsize % MMC_DFLT_BLKSIZE) == 0) {
		uint32_t ts = bsize / MMC_DFLT_BLKSIZE;
		csize = csize * ts;
		bsize = bsize / ts;
	}

	lba = csize * csizem - 1;

	ext->mmc_blksize		= bsize;
	cptr->lba				= ENDIAN_BE32(lba);
	cptr->blk_size			= ENDIAN_BE32(bsize);

	ccb->cam_ch.cam_status	= CAM_REQ_CMP;
	ccb->cam_scsi_status	= SCS_GOOD;
}

static void mmc_devctl(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_DEVCTL	*ccb = (CCB_DEVCTL *)ext->nexus;

	ccb->cam_devctl_status = EOK;
	switch (ccb->cam_devctl_dcmd) {
		default:
			ccb->cam_devctl_status = _RESMGR_DEFAULT;
			break;
	}
	ccb->cam_ch.cam_status = CAM_REQ_CMP;
}

paddr_t mmc_mphys(CCB_SCSIIO *ccb, paddr_t dptr, int sgi)
{
	mdl_t	*mdl;
	int		cnt;
	ioreq_t	*ioreq;
	off64_t	_off;

	if (ccb->cam_ch.cam_flags & CAM_DATA_PHYS)
		return (dptr);

	if (ioreq = (ioreq_t *)ccb->cam_req_map) {
		mdl = ioreq->mdl;
		if (mdl[sgi].vaddr == (caddr_t)dptr)
			return (mdl[sgi].paddr);
 
		for (cnt = ioreq->nmdl; cnt; cnt--, mdl++) {
			if (mdl->vaddr == (caddr_t)dptr)
				return (mdl->paddr);
		}
	}

	mem_offset64((void *)(dptr), NOFD, 1, &_off, NULL);

	return ((paddr_t)_off);
}

static int mmc_stop_xfer(SIM_HBA *hba)
{
	return mmc_sendcmd(hba, MMC_STOP_TRANSMISSION, MMC_RSP_R1B, MMC_CMD_INTR, 0);
}

static int mmc_pio_xfer(SIM_HBA *hba, uint32_t nbytes)
{
	SIM_MMC_EXT	*ext;

	ext	= (SIM_MMC_EXT *)hba->ext;

	if (ext->setup_pio(hba, nbytes, ext->dir) == nbytes)
		return mmc_send_rwcmd(hba, nbytes, 0);

	return 0;
}

static int mmc_dma_xfer(SIM_HBA *hba, paddr_t paddr, uint32_t nbytes)
{
	SIM_MMC_EXT	*ext;

	ext	= (SIM_MMC_EXT *)hba->ext;

	if ((nbytes = ext->setup_dma(hba, paddr, nbytes, ext->dir)) > 0)
		return mmc_send_rwcmd(hba, nbytes, MMC_CMD_DATA_DMA);

	return 0;
}

static void mmc_dma_rw(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;
	uint32_t	nbytes;

	ext	= (SIM_MMC_EXT *)hba->ext;
	ccb	= ext->nexus;

	if (ext->nbytes == ext->offset) {	// All done
		ccb->cam_ch.cam_status = CAM_REQ_CMP;
		ccb->cam_scsi_status   = SCS_GOOD;
		return;
	}

	if ((nbytes = mmc_dma_xfer(hba, ext->paddr + ext->offset, ext->nbytes - ext->offset)) > 0) {
		ext->offset += nbytes;
		ext->blkno  += nbytes / ext->mmc_blksize;
	} else
		mmc_error(hba, CAM_PROVIDE_FAIL);
}

static inline void mmc_pio_initiate(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;
	uint32_t	blkno, nbytes = 0;

	ext		= (SIM_MMC_EXT *)hba->ext;
	ccb		= ext->nexus;
	blkno	= ext->blkno;

	if (ccb->cam_ch.cam_flags & CAM_DATA_PHYS) {
		mmc_error(hba, CAM_PROVIDE_FAIL);
		return;
	}

	if (ccb->cam_ch.cam_flags & CAM_SCATTER_VALID) {
		SG_ELEM		*sge = ccb->cam_data.cam_sg_ptr;
		uint32_t	sgi;

		for (sgi = 0; sgi < ccb->cam_sglist_cnt; sgi++, sge++)
			nbytes += sge->cam_sg_count;
	} else
		nbytes = ccb->cam_dxfer_len;

	ext->nbytes = nbytes;
	if (mmc_pio_xfer(hba, nbytes) > 0) {
		ext->state = MMCSD_STATE_DATA;
		ccb->cam_ch.cam_status = CAM_REQ_INPROG;
		return;
	}

	mmc_error(hba, CAM_PROVIDE_FAIL);
}

static inline void mmc_dma_initiate(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;
	uint32_t	nbytes;

	ext	= (SIM_MMC_EXT *)hba->ext;
	ccb	= ext->nexus;

	if (ccb->cam_ch.cam_flags & CAM_SCATTER_VALID) {
		if (ext->setup_sgdma) {
			if (nbytes = ext->setup_sgdma(hba, ext->dir)) {
				if (mmc_send_rwcmd(hba, nbytes, MMC_CMD_DATA_DMA) == nbytes) {
					ccb->cam_ch.cam_status = CAM_REQ_INPROG;
					ext->state = MMCSD_STATE_DATA;
					ext->nbytes = ext->offset = nbytes;	// all done

					return;
				}
			}
		}

		mmc_error(hba, CAM_PROVIDE_FAIL);
	} else {
		ccb->cam_ch.cam_status = CAM_REQ_INPROG;

		ext->paddr = mmc_mphys(ccb, ccb->cam_data.cam_data_ptr, 0);
		ext->nbytes = ccb->cam_dxfer_len;

		mmc_dma_rw(hba);

		if (ccb->cam_ch.cam_status == CAM_REQ_INPROG)
			ext->state = MMCSD_STATE_DATA;
	}
}

static void mmc_pio_stop(SIM_HBA *hba, int dir)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;

	ext = (SIM_MMC_EXT *)hba->ext;
	ccb = ext->nexus;

	if (ext->dir == dir) {
		if (ext->stop == MMCSD_STOP_PEND) {
			if (mmc_stop_xfer(hba) != MMC_SUCCESS) {
				mmc_error(hba, CAM_PROVIDE_FAIL);
				ext->stop = MMCSD_STOP_NONE;
				return;
			}

			ext->stop = MMCSD_STOP_ISSUED;
		} else {
			ccb->cam_ch.cam_status = CAM_REQ_CMP;
			ccb->cam_scsi_status   = SCS_GOOD;
		}
	}
}

static void mmc_pio_rw(SIM_HBA *hba, int status)
{
	SIM_MMC_EXT	*ext;
	CCB_SCSIIO	*ccb;
	uint32_t	nbytes;

	ext = (SIM_MMC_EXT *)hba->ext;
	ccb = ext->nexus;

	if (ext->offset == -1)
		return mmc_pio_stop(hba, MMC_DIR_OUT);

	if (ccb->cam_ch.cam_flags & CAM_SCATTER_VALID) {
		SG_ELEM		*sge = ccb->cam_data.cam_sg_ptr;
		uint32_t	sgi = ext->cur_sg;

		do {
			nbytes = ext->pio_done(hba, (char *)sge[sgi].cam_sg_address + ext->offset,
						sge[sgi].cam_sg_count - ext->offset, ext->dir);
			ext->offset += nbytes;
			if (ext->offset == sge[sgi].cam_sg_count) {
				if (++sgi >= ccb->cam_sglist_cnt)		// All done
					ext->offset = -1;
				else
					ext->offset = 0;
			}
		} while (nbytes > 0 && ext->offset != -1);

		ext->cur_sg = sgi;
	} else {
		do {
			nbytes = ext->pio_done(hba, (char *)ccb->cam_data.cam_data_ptr + ext->offset,
						ccb->cam_dxfer_len - ext->offset, ext->dir);

			ext->offset += nbytes;
			if (ext->offset == ext->nbytes)
				ext->offset = -1;
		} while (nbytes > 0 && ext->offset != -1);
	}

	if (ext->offset == -1)
		return mmc_pio_stop(hba, MMC_DIR_IN);
}

// interpret SCSI commands
void mmc_io_cmds(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;
	uint32_t	blkno;

	if (ext->state == MMCSD_STATE_IDENT)
		return;

	switch (ccb->cam_cdb_io.cam_cdb_bytes[0]) {
		case SC_MSENSE10:
			mmc_msense(hba);
			break;

		case SC_UNIT_RDY:
			mmc_unit_ready(hba);
			break;

		case SC_INQUIRY:
			mmc_inquiry(hba);
			break;

		case SC_RD_CAP:
			mmc_capacity(hba);
			break;

		case SC_SPINDLE:
			mmc_spindle(hba);
			break;

		case SC_SYNC:
			ccb->cam_ch.cam_status  = CAM_REQ_CMP;
			ccb->cam_scsi_status    = SCS_GOOD;
			break;

		case SC_WRITE10:
			if (ext->eflags & MMC_EFLAG_WP) {		// Write protected
				mmc_sense(hba, SK_DATA_PROT, ASC_WRITE_PROTECTED, 0);
				break;
			}
		case SC_READ10:
			if (ext->eflags & MMC_EFLAG_READY) {
				ext->offset = 0;
				ext->cur_sg = 0;
				ext->dir    = ccb->cam_cdb_io.cam_cdb_bytes[0] == SC_READ10 ? MMC_DIR_IN : MMC_DIR_OUT;
//				ext->hba    = hba;
				blkno       = UNALIGNED_RET32(&ccb->cam_cdb_io.cam_cdb_bytes[2]);
				ext->blkno  = ENDIAN_BE32(blkno);
				if (ext->eflags & MMC_EFLAG_PIO) {
					if (ext->setup_pio)
						mmc_pio_initiate(hba);
					else
						mmc_error(hba, CAM_PROVIDE_FAIL);
				} else {
					if (ext->setup_dma)
						mmc_dma_initiate(hba);
					else
						mmc_error(hba, CAM_PROVIDE_FAIL);
				}
			} else {
				mmc_sense(hba, SK_NOT_RDY, ASC_MEDIA_NOT_PRESENT, 0);
			}
			break;

		default:
			mmc_sense(hba, SK_ILLEGAL, 0x24, 0);
			break;
	}
}

// start executing a new CCB
static void mmc_start_ccb(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	CCB_SCSIIO		*ccb;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (ext->nexus && !(simq_ccb_state(ext->nexus, SIM_CCB_QUERY) & SIM_CCB_ABORT))
		return;

	if ((ext->nexus = ccb = simq_ccb_dequeue(hba->simq)) == NULL)
		return;

	switch (ccb->cam_ch.cam_func_code) {
		case XPT_SCSI_IO:
			mmc_io_cmds(hba);
			break;

		case XPT_DEVCTL:
			mmc_devctl(hba);
			break;

		default:
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR,
					"mmc_start_ccb: unsupported function code %d", ccb->cam_ch.cam_func_code);
			ccb->cam_ch.cam_status	= CAM_REQ_CMP_ERR;
			ccb->cam_scsi_status	= SCS_CHECK;
			break;
	}

	if (ccb->cam_ch.cam_status != CAM_REQ_INPROG) {
		ext->nexus = NULL;
		simq_post_ccb(hba->simq, ccb);
	}
}

static void mmc_dma_irq(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;

	if (ext->stop == MMCSD_STOP_PEND) {
		if (mmc_stop_xfer(hba) != MMC_SUCCESS) {
			mmc_error(hba, CAM_PROVIDE_FAIL);
			ext->stop = MMCSD_STOP_NONE;
			return;
		}
		ext->stop = MMCSD_STOP_ISSUED;
		return;
	}

	mmc_dma_rw(hba);
}

static void mmc_ident_irq(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;

	if (mmc_card_ident(hba, intr) == MMC_SUCCESS) {
		if (ext->ident == MMCSD_IDENT_NONE) {	// ident complete
			ext->state = MMCSD_STATE_IDLE;
			if(!(ext->eflags & MMC_EFLAG_READY)){
				atomic_set(&ext->eflags, MMC_EFLAG_READY);
				MsgSendPulse(mmc_ctrl.coid, MMC_PRIORITY, MMCSD_PULSE_CARD_READY, 0);
			}
			if (ccb)
				mmc_io_cmds(hba);
		}
	} else {
		ext->state = MMCSD_STATE_IDLE;

		if (ext->detect(hba) == MMC_SUCCESS) {
			// unsupported card
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "Unsupported card inserted!");
			atomic_set(&ext->eflags, MMC_EFLAG_INVALID_CARD);
			ext->powerdown(hba);	// powerdown the card
		} else {
			// card removed before ident is done
			mmc_media_check(hba);
		}

		if (ccb)
			mmc_error(hba, CAM_PROVIDE_FAIL);
	}

}

static void mmc_data_irq(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;
	static int 	stops_retry=0;

	if (ext->stop == MMCSD_STOP_ISSUED) {			// stop command issued ?
		if (intr & MMC_INTR_ERROR) {				// error ?
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR,
					"mmc_process_interrupt: stop command error %x", intr);
			if(stops_retry++>5 || (mmc_stop_xfer(hba) != MMC_SUCCESS)){
				mmc_error(hba, CAM_PROVIDE_FAIL);
				ext->stop = MMCSD_STOP_NONE;
			}
		} else if (intr & MMC_INTR_COMMAND) {		// command complete ?
			stops_retry = 0;
			ext->stop = MMCSD_STOP_NONE;
			if (ext->eflags & MMC_EFLAG_PIO) {
				ccb->cam_ch.cam_status = CAM_REQ_CMP;
				ccb->cam_scsi_status   = SCS_GOOD;
			} else
				mmc_dma_irq(hba);
		}
	} else {
		stops_retry = 0;
		if (intr & MMC_INTR_ERROR) {
			mmc_error(hba, CAM_PROVIDE_FAIL);
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR,
					"mmc_process_interrupt: data error %x", intr);
		} else if (intr & MMC_INTR_DATA) {
			if (ext->eflags & MMC_EFLAG_PIO)
				return mmc_pio_rw(hba, intr);

			if (ext->dma_done(hba, ext->dir) == MMC_SUCCESS)
				return mmc_dma_irq(hba);

			mmc_error(hba, CAM_PROVIDE_FAIL);
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR,
					"mmc_process_interrupt: data error %x", intr);
		} else if (intr & (MMC_INTR_RBRDY | MMC_INTR_WBRDY))
			mmc_pio_rw(hba, intr);
	}
}

static void mmc_media_check(SIM_HBA *hba)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;

	if (ext->detect(hba) == MMC_SUCCESS) {
		if (!(atomic_set_value(&ext->eflags, MMC_EFLAG_INSERTED) & MMC_EFLAG_INSERTED)) {
//			atomic_set(&ext->eflags, MMC_EFLAG_MEDIA_CHANGED);
			// Insertion
			if (hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMCSD : card inserted");
			MsgSendPulse(mmc_ctrl.coid, MMC_PRIORITY, MMCSD_PULSE_CARD_INSERTION, 0);
		}
	} else {
		if (atomic_clr_value(&ext->eflags, MMC_EFLAG_INSERTED) & MMC_EFLAG_INSERTED) {
			// Removal
			if (hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMCSD : card removed");
			atomic_clr(&ext->eflags, MMC_EFLAG_READY | MMC_EFLAG_INVALID_CARD);
			ext->powerdown(hba);	// powerdown the card
			MsgSendPulse(mmc_ctrl.coid, MMC_PRIORITY, MMCSD_PULSE_CARD_REMOVAL, 0);
		}
	}
}

static int mmc_process_interrupt(SIM_HBA *hba, int irq)
{
	SIM_MMC_EXT	*ext = (SIM_MMC_EXT *)hba->ext;
	CCB_SCSIIO	*ccb = ext->nexus;
	uint32_t	intr;

	if ((intr = ext->interrupt(hba, irq, ext->resp_type, ext->mmc_resp)) == 0)
		return 0;

	if (intr & MMC_INTR_CARD) {
		mmc_media_check(hba);

		mmc_error(hba, CAM_PROVIDE_FAIL);
	} else {
		switch (ext->state) {
		case MMCSD_STATE_IDENT:
			mmc_ident_irq(hba, intr);
			break;
		case MMCSD_STATE_DATA:
			mmc_data_irq(hba, intr);
			break;
		default:
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO,
				"MMCSD : unexpected interrupt, state = %d, intr status = %x", ext->state, intr);
			break;
		}
	}

	if (ccb) {
		if (ccb->cam_ch.cam_status != CAM_REQ_INPROG) {
			ext->nexus = NULL;
			ext->state = MMCSD_STATE_IDLE;
			simq_post_ccb(hba->simq, ccb);
		}
	}

	return 0;
}

// the driver thread starts here
static inline void mmc_pulse_handler(SIM_HBA *hba)
{
	struct _pulse	pulse;
	iov_t			iov;
	int				rcvid;
	SIM_MMC_EXT		*ext;

	ext = (SIM_MMC_EXT *)hba->ext;

	SETIOV(&iov, &pulse, sizeof(pulse));

	while (1) {
		if ((rcvid = MsgReceivev(hba->chid, &iov, 1, NULL)) == -1)
			continue;
		switch (pulse.code) {
			case SIM_ENQUEUE:
				mmc_start_ccb(hba);
				break;

			case SIM_INTERRUPT:
				mmc_process_interrupt(hba, hba->cfg.IRQRegisters[0]);
				InterruptUnmask(hba->cfg.IRQRegisters[0], hba->iid);
				continue;

			case SIM_DMA_INTERRUPT:
				mmc_process_interrupt(hba, hba->cfg.IRQRegisters[1]);
				InterruptUnmask(hba->cfg.IRQRegisters[1], ext->iid);
				continue;

			case SIM_TIMER:
				// Only check media if card detection interrupt is not supported
				if (!(ext->hccap & MMC_HCCAP_CD_INTR))
					mmc_media_check(hba);
				break;

			default:
				if (rcvid)
					MsgReplyv(rcvid, ENOTSUP, &iov, 1);
				break;
		}

		if (ext->nexus == NULL)
			mmc_start_ccb(hba);
	}
}

void *mmc_driver_thread(void *data)
{ 
	SIM_HBA			*hba;
	SIM_MMC_EXT		*ext;
	struct sigevent	event;

	hba = (SIM_HBA *)data;
	ext = (SIM_MMC_EXT *)hba->ext;

	if (hba->cfg.NumIRQs) {
		SIGEV_PULSE_INIT(&event, hba->coid, MMC_IRQ_PRIORITY, SIM_INTERRUPT, NULL);

		if ((hba->iid = InterruptAttachEvent(hba->cfg.IRQRegisters[0], &event, _NTO_INTR_FLAGS_TRK_MSK)) == -1)
			perror("Interrupt attach failed:");

		if (hba->cfg.NumIRQs > 1) {
			SIGEV_PULSE_INIT(&event, hba->coid, MMC_IRQ_PRIORITY, SIM_DMA_INTERRUPT, NULL);

			if ((ext->iid = InterruptAttachEvent(hba->cfg.IRQRegisters[1], &event, _NTO_INTR_FLAGS_TRK_MSK)) == -1)
				perror("Interrupt attach failed:");
		}
	}

	atomic_set(&ext->eflags, MMC_EFLAG_DRIVER_READY);

	while (1) {
		if (ext->eflags & MMC_EFLAG_MONITOR_READY)
			break;
		delay(1);
	}

	mmc_pulse_handler(hba);

	return (CAM_SUCCESS);
}

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devb/mmcsd/sim_mmc.c $ $Rev: 249398 $" );

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

//#define MMCSD_DEBUG

static const uint32_t mmcsd_ts_exp[] = { 100, 1000, 10000, 100000, 0, 0, 0, 0 };
static const uint32_t mmcsd_ts_mul[] = { 0, 1000, 1200, 1300, 1500, 2000, 2500, 3000,
						3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };


char *mmc_idents[] = {	"MMCSD_IDENT_NONE", "MMCSD_IDENT_START",
						"MMCSD_IDENT_GO_IDLE", "MMCSD_IDENT_CARD_VERSION",
						"MMCSD_IDENT_SD_POWERUP", "MMCSD_IDENT_MMC_POWERUP"
						"MMCSD_IDENT_ALL_SEND_CID", "MMCSD_IDENT_SET_ADDRESS",
						"MMCSD_IDENT_SEND_CID", "MMCSD_IDENT_SEND_CSD",
						"MMCSD_IDENT_SEND_SCR", "MMCSD_IDENT_PARSE_SCR",
						"MMCSD_IDENT_CHECK_MODE", "MMCSD_IDENT_SWITCH_MODE",
						"MMCSD_IDENT_SEL_DES_CARD", "MMCSD_IDENT_CHECK_STS",
						"MMCSD_IDENT_SET_BW_APP", "MMCSD_IDENT_SET_BW",
						"MMCSD_IDENT_FINAL" };

char *mmc_cmds[] = {	"MMC_GO_IDLE_STATE", "MMC_SEND_OP_COND",
						"MMC_ALL_SEND_CID", "MMC_SET_RELATIVE_ADDR",
						"MMC_SET_DSR", "", "SD_SET_BUS_WIDTH",
						"MMC_SEL_DES_CARD", "MMC_IF_COND", "MMC_SEND_CSD",
						"MMC_SEND_CID", "MMC_READ_DAT_UNTIL_STOP",
						"MMC_STOP_TRANSMISSION", "MMC_SEND_STATUS", "",
						"MMC_GO_INACTIVE_STATE", "MMC_SET_BLOCKLEN",
						"MMC_READ_SINGLE_BLOCK", "MMC_READ_MULTIPLE_BLOCK",
						"", "MMC_WRITE_DAT_UNTIL_STOP", "", "", "",
						"MMC_WRITE_BLOCK", "MMC_WRITE_MULTIPLE_BLOCK",
						"MMC_PROGRAM_CID", "MMC_PROGRAM_CSD",
						"MMC_SET_WRITE_PROT", "MMC_CLR_WRITE_PROT",
						"MMC_SEND_WRITE_PROT", "", "MMC_TAG_SECTOR_START",
						"MMC_TAG_SECTOR_END", "MMC_UNTAG_SECTOR",
						"MMC_TAG_ERASE_GROUP_START", "MMC_TAG_ERASE_GROUP_END",
						"MMC_UNTAG_ERASE_GROUP", "MMC_ERASE", "MMC_FAST_IO",
						"MMC_GO_IRQ_STATE", "SD_SEND_OP_COND", "MMC_LOCK_UNLOCK",
						"", "", "", "", "", "", "", "", "SD_SEND_SCR", "", "",
						"", "MMC_APP_CMD", "MMC_GEN_CMD", "", "MMC_READ_OCR",
						"MMC_CRC_ON_OFF" };


int _mmc_sendcmd(SIM_HBA *hba, uint32_t cmdflg, uint32_t argument)
{
	SIM_MMC_EXT		*ext;
	mmc_cmd_t		cmd;

	ext = (SIM_MMC_EXT *)hba->ext;

	/* Put the card in idle state */
	cmd.argument = argument;
	cmd.opcode   = cmdflg & 0xFF;
	cmd.resp     = (cmdflg >> 8) & 0xFF;
	cmd.eflags   = (cmdflg >> 16) & 0xFFFF;
	if (ext->eflags & MMC_EFLAG_PPL)
		cmd.eflags  |= MMC_CMD_PPL;

	ext->mmc_resp[0] = ext->mmc_resp[1] = ext->mmc_resp[2] = ext->mmc_resp[3] = 0;
	ext->resp_type = cmd.resp;

#ifdef MMCSD_DEBUG
	mmc_slogf( hba, _SLOGC_SIM_MMC, _SLOG_ERROR, 1, "%s: %s (%d), arg 0x%x, resp 0x%x, eflags 0x%x", __FUNCTION__, mmc_cmds[cmd.opcode], cmd.opcode, cmd.argument, cmd.resp, cmd.eflags );
#endif

	return (ext->command(hba, &cmd));
}

int mmc_send_rwcmd(SIM_HBA *hba, int nbytes, int flag)
{
	SIM_MMC_EXT	*ext;
	uint32_t	cmd;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (nbytes == ext->mmc_blksize) {
		cmd = ext->dir == MMC_DIR_IN ? MMC_READ_SINGLE_BLOCK : MMC_WRITE_BLOCK;
		ext->stop = MMCSD_STOP_NONE;
	} else {
		cmd = ext->dir == MMC_DIR_IN ? MMC_READ_MULTIPLE_BLOCK : MMC_WRITE_MULTIPLE_BLOCK;
		if (!(ext->hccap & MMC_HCCAP_ACMD12))
			ext->stop = MMCSD_STOP_PEND;
		else
			ext->stop = MMCSD_STOP_AUTO;
		flag |= MMC_CMD_DATA_MULTI;
	}

	if (ext->dir == MMC_DIR_IN)
		flag |= MMC_CMD_DATA_IN;

	flag |= MMC_CMD_DATA;

	/*
	 * Data is expected
	 * DON'T set MMC_CMD_INTR flag
	 */
	if (mmc_sendcmd(hba, cmd, MMC_RSP_R1, flag,	
		(ext->eflags & MMC_EFLAG_HC) ? ext->blkno : (ext->blkno * ext->mmc_blksize)) != MMC_SUCCESS)
		return 0;

	return nbytes;
}



static void mmc_disp_mmc_cid(uint8_t mmc_prot, mmc_cid_t *cid)
{
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC CID info:");
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MID (Manufacturer ID)       : %d", cid->mid);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PNM (Product name)          : %s", cid->pnm);
	if (mmc_prot < 2) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " HWR (Hardware revision)     : %d", cid->hwr);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " FWR (Hardware revision)     : %d", cid->fwr);
	} else
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " OID (OEM ID)                : %d", cid->oid);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PSN (Product serial number) : %08x", cid->psn);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MCD (Month code)            : %d", cid->mcd);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " YCD (Year code)             : %d", cid->ycd);
}

/*
 * CSD has to be already parsed
 */
void mmc_parse_mmc_cid(SIM_HBA *hba, mmc_cid_t *cid, uint32_t *resp)
{
	SIM_MMC_EXT		*ext;

	ext = (SIM_MMC_EXT *)hba->ext;

	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = resp[1] >> 24;

	if (ext->csd.mmc_csd.mmc_prot < 2) {
		cid->mid    = resp[3] >> 8;
		cid->pnm[6] = resp[1] >> 16;
		cid->pnm[7] = 0;
		cid->hwr    = (resp[1] >> 12) & 0x0F;
		cid->fwr    = (resp[1] >> 8) & 0x0F;
	} else {
		cid->mid    = resp[3] >> 24;
		cid->oid    = (resp[3] >> 8) & 0xFFFF;
		cid->pnm[6] = 0;
	}

	cid->psn    = ((resp[1] & 0xFFFF) << 16) | (resp[0] >> 16);
	cid->mcd    = (resp[0] >> 12) & 0x0F;
	cid->ycd    = ((resp[0] >> 8) & 0x0F) + 1997;

	if (hba->verbosity > 1)
		mmc_disp_mmc_cid(ext->csd.mmc_csd.mmc_prot, cid);
}

static void mmc_disp_sd_cid(sd_cid_t *cid)
{
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD CID info:");
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MID (Manufacturer ID)       : %d", cid->mid);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " OID (OEM/Application ID)    : %s", cid->oid);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PNM (Product name)          : %s", cid->pnm);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PRV (Product revision)      : %d.%d", cid->prv >> 4, cid->prv & 0x0F);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PSN (Product serial number) : %08x", cid->psn);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MDT (Manufacturing date)    : %d.%d", ((cid->mdt >> 4) & 0xFF) + 2000, cid->mdt & 0x0F);
}

void mmc_parse_sd_cid(SIM_HBA *hba, sd_cid_t *cid, uint32_t *resp)
{
	cid->mid    = resp[3] >> 24;
	cid->oid[0] = (resp[3] >> 8) & 0xFF;
	cid->oid[1] = (resp[3] >> 16) & 0xFFFF;
	cid->oid[2] = 0;
	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = 0;
	cid->prv    = resp[1] >> 24;
	cid->psn    = (resp[1] << 8) | (resp[0] >> 24);
	cid->mdt    = (resp[0] >> 8) & 0xFFFF;

	if (hba->verbosity > 1)
		mmc_disp_sd_cid(cid);
}

static void mmc_disp_mmc_csd(mmc_csd_t *csd)
{
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC CSD info:");
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " CSD_STRUCTURE      : %x", csd->csd_structure);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MMC_PROT           : %x", csd->mmc_prot);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TAAC               : %x", csd->taac);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " NSAC               : %x", csd->nsac);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TRAN_SPEED         : %x", csd->tran_speed);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " CCC                : %x", csd->ccc);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BL_LEN        : %x", csd->read_bl_len);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BL_PARTIAL    : %x", csd->read_bl_partial);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BLK_MISALIGN : %x", csd->write_blk_misalign);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BLK_MISALIGN  : %x", csd->read_blk_misalign);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " DSR_IMP            : %x", csd->dsr_imp);
	if (csd->csd_structure <= CSD_VERSION_11) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SECTOR_SIZE        : %x", csd->erase.mmc_v22.sector_size);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " ERASE_GRP_SIZE     : %x", csd->erase.mmc_v22.erase_grp_size);
	} else  {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " ERASE_GRP_SIZE     : %x", csd->erase.mmc_v31.erase_grp_size);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " ERASE_GRP_MULT     : %x", csd->erase.mmc_v31.erase_grp_mult);
	}
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WP_GRP_SIZE        : %x", csd->wp_grp_size);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " R2W_FACTOR         : %x", csd->r2w_factor);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BL_LEN       : %x", csd->write_bl_len);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BL_PARTIAL   : %x", csd->write_bl_partial);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " COPY               : %x", csd->copy);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PERM_WRITE_PROTECT : %x", csd->perm_write_protect);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TMP_WRITE_PROTECT  : %x", csd->tmp_write_protect);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " ECC                : %x", csd->ecc);
}

void mmc_parse_mmc_csd(SIM_HBA *hba, mmc_csd_t *csd, uint32_t *resp)
{
	csd->csd_structure      = resp[3] >> 30;
	csd->mmc_prot			= (resp[3] >> 26) & 0x0F;
	csd->taac               = resp[3] >> 16;
	csd->nsac               = resp[3] >> 8;
	csd->tran_speed         = resp[3];
	csd->ccc                = resp[2] >> 20;
	csd->read_bl_len        = (resp[2] >> 16) & 0x0F;
	csd->read_bl_partial    = (resp[2] >> 15) & 1;
	csd->write_blk_misalign = (resp[2] >> 14) & 1;
	csd->read_blk_misalign  = (resp[2] >> 13) & 1;
	csd->dsr_imp            = (resp[2] >> 12) & 1;
	csd->c_size				= ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
	csd->vdd_r_curr_min		= (resp[1] >> 27) & 0x07;
	csd->vdd_r_curr_max		= (resp[1] >> 24) & 0x07;
	csd->vdd_w_curr_min		= (resp[1] >> 21) & 0x07;
	csd->vdd_w_curr_max		= (resp[1] >> 18) & 0x07;
	csd->c_size_mult		= (resp[1] >> 15) & 0x07;

	if (csd->csd_structure <= CSD_VERSION_11) {
		csd->erase.mmc_v22.sector_size		= (resp[1] >> 10) & 0x1F;
		csd->erase.mmc_v22.erase_grp_size	= (resp[1] >> 5) & 0x1F;
	} else {
		csd->erase.mmc_v31.erase_grp_size	= (resp[1] >> 10) & 0x1F;
		csd->erase.mmc_v31.erase_grp_mult	= (resp[1] >> 5) & 0x1F;
	}
	csd->wp_grp_size        = (resp[1] >> 0) & 0x1F;
	csd->wp_grp_enable      = (resp[0] >> 31);
	csd->r2w_factor         = (resp[0] >> 26) & 0x07;
	csd->write_bl_len       = (resp[0] >> 22) & 0x0F;
	csd->write_bl_partial   = (resp[0] >> 21) & 1;
	csd->copy               = (resp[0] >> 14) & 1;
	csd->perm_write_protect = (resp[0] >> 13) & 1;
	csd->tmp_write_protect  = (resp[0] >> 12) & 1;
	csd->ecc				= (resp[0] >>  8) & 3;

	if (csd->perm_write_protect || csd->tmp_write_protect) {
		SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
		ext->eflags |= MMC_EFLAG_WP;
	}

	if (hba->verbosity > 1)
		mmc_disp_mmc_csd(csd);
}

static void mmc_disp_sd_csd(sd_csd_t *csd)
{
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD CSD info:");
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " CSD_STRUCTURE      : %x", csd->csd_structure);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TAAC               : %x", csd->taac);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " NSAC               : %x", csd->nsac);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TRAN_SPEED         : %x", csd->tran_speed);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " CCC                : %x", csd->ccc);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BL_LEN        : %x", csd->read_bl_len);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BL_PARTIAL    : %x", csd->read_bl_partial);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BLK_MISALIGN : %x", csd->write_blk_misalign);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " READ_BLK_MISALIGN  : %x", csd->read_blk_misalign);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " DSR_IMP            : %x", csd->dsr_imp);
	if (csd->csd_structure == 0) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " C_SIZE             : %x", csd->csd.csd_ver1.c_size);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " VDD_R_CURR_MIN     : %x", csd->csd.csd_ver1.vdd_r_curr_min);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " VDD_R_CURR_MAX     : %x", csd->csd.csd_ver1.vdd_r_curr_max);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " VDD_W_CURR_MIN     : %x", csd->csd.csd_ver1.vdd_w_curr_min);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " VDD_W_CURR_MAX     : %x", csd->csd.csd_ver1.vdd_w_curr_max);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " C_SIZE_MULT        : %x", csd->csd.csd_ver1.c_size_mult);
	} else
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " C_SIZE             : %x", csd->csd.csd_ver2.c_size);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " ERASE_BLK_EN       : %x", csd->erase_blk_en);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SECTOR_SIZE        : %x", csd->sector_size);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WP_GRP_SIZE        : %x", csd->wp_grp_size);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " R2W_FACTOR         : %x", csd->r2w_factor);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BL_LEN       : %x", csd->write_bl_len);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " WRITE_BL_PARTIAL   : %x", csd->write_bl_partial);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " FILE_FORMAT_GRP    : %x", csd->file_format_grp);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " COPY               : %x", csd->copy);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " PERM_WRITE_PROTECT : %x", csd->perm_write_protect);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " TMP_WRITE_PROTECT  : %x", csd->tmp_write_protect);
	slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " FILE_FORMAT        : %x", csd->file_format);
}

void mmc_parse_sd_csd(SIM_HBA *hba, sd_csd_t *csd, uint32_t *resp)
{
	csd->csd_structure      = resp[3] >> 30;
	csd->taac               = resp[3] >> 16;
	csd->nsac               = resp[3] >> 8;
	csd->tran_speed         = resp[3];
	csd->ccc                = resp[2] >> 20;
	csd->read_bl_len        = (resp[2] >> 16) & 0x0F;
	csd->read_bl_partial    = (resp[2] >> 15) & 1;
	csd->write_blk_misalign = (resp[2] >> 14) & 1;
	csd->read_blk_misalign  = (resp[2] >> 13) & 1;
	csd->dsr_imp            = (resp[2] >> 12) & 1;

	if (csd->csd_structure == 0) {
		csd->csd.csd_ver1.c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
		csd->csd.csd_ver1.vdd_r_curr_min = (resp[1] >> 27) & 0x07;
		csd->csd.csd_ver1.vdd_r_curr_max = (resp[1] >> 24) & 0x07;
		csd->csd.csd_ver1.vdd_w_curr_min = (resp[1] >> 21) & 0x07;
		csd->csd.csd_ver1.vdd_w_curr_max = (resp[1] >> 18) & 0x07;
		csd->csd.csd_ver1.c_size_mult    = (resp[1] >> 15) & 0x07;
	} else {
		csd->csd.csd_ver2.c_size = ((resp[2] & 0x3F) << 16) | (resp[1] >> 16);
	}
	csd->erase_blk_en       = (resp[1] >> 14) & 1;
	csd->sector_size        = (resp[1] >> 7) & 0x7F;
	csd->wp_grp_size        = (resp[1] >> 0) & 0x7F;
	csd->wp_grp_enable      = (resp[0] >> 31);
	csd->r2w_factor         = (resp[0] >> 26) & 0x07;
	csd->write_bl_len       = (resp[0] >> 22) & 0x0F;
	csd->write_bl_partial   = (resp[0] >> 21) & 1;
	csd->file_format_grp    = (resp[0] >> 15) & 1;
	csd->copy               = (resp[0] >> 14) & 1;
	csd->perm_write_protect = (resp[0] >> 13) & 1;
	csd->tmp_write_protect  = (resp[0] >> 12) & 1;
	csd->file_format        = (resp[0] >> 10) & 3;

	if (csd->perm_write_protect || csd->tmp_write_protect) {
		SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
		ext->eflags |= MMC_EFLAG_WP;
	}

	if (hba->verbosity > 1)
		mmc_disp_sd_csd(csd);
}

static uint32_t mmcsd_tran_speed(uint8_t ts)
{
	return mmcsd_ts_exp[(ts & 0x7)] * mmcsd_ts_mul[(ts & 0x78) >> 3];
}

static int mmc_ident_sd_clock(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
	int				clock, hs = 0;

	if (ext->eflags & MMC_EFLAG_FORCECLK)
		clock = ext->nclock;
	else {
		clock = mmcsd_tran_speed(ext->csd.sd_csd.tran_speed);
		if (ext->eflags & MMC_EFLAG_HS) {
			clock = 50000000;
			hs = 1;
		}
	}

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD : config clock as %d", clock);
	ext->set_clock(hba, &clock, hs);

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD : actual operation clock %d", clock);

	ext->set_blksz(hba, MMC_DFLT_BLKSIZE);	// block size, 512, every card should support

	ext->ident = MMCSD_IDENT_FINAL;

	return mmc_sendcmd(hba, MMC_SET_BLOCKLEN, MMC_RSP_R1, MMC_CMD_INTR, MMC_DFLT_BLKSIZE);
}

static int mmc_ident_mmc_clock(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
	int				clock;
	if (ext->eflags & MMC_EFLAG_FORCECLK)
		clock = ext->nclock;		// MMC operation clock
	else
		clock = mmcsd_tran_speed(ext->csd.mmc_csd.tran_speed);
	
	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO,
			"MMC config: clock %d, bus width %d, block size %d", clock, 1, MMC_DFLT_BLKSIZE);
	
	ext->cfg_bus(hba, 1, 1);		// 1 bit bus, MMC card
	ext->set_blksz(hba, MMC_DFLT_BLKSIZE);	// block size, 512, every card should support
	ext->set_clock(hba, &clock, 0); // no high speed support
	
	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : actual operation clock %d", clock);
	
	ext->ident = MMCSD_IDENT_FINAL;
	
	/*
	 * Set bus width?
	 * We need MMC specification
	 */
	return mmc_sendcmd(hba, MMC_SET_BLOCKLEN, MMC_RSP_R1, MMC_CMD_INTR, MMC_DFLT_BLKSIZE);
}

static int mmc_ident_switch_buswidth(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR){
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : Switch to bus width %d bits failed", ext->bus_width);
		return (CAM_FAILURE);
	}

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : Switch to bus width %d bits successful", ext->bus_width);

	ext->set_blksz(hba, MMC_DFLT_BLKSIZE);	// block size, 512, every card should support
	ext->ident = MMCSD_IDENT_FINAL;

	return mmc_sendcmd(hba, MMC_SET_BLOCKLEN, MMC_RSP_R1, MMC_CMD_INTR, MMC_DFLT_BLKSIZE);
}

static int mmc_ident_switch_mode_hs(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;
	int				clock, hs = 0;
	ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR){
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : Switch to hih speed mode failed");
		return (CAM_FAILURE);
	}

	/*we are in HS now*/
	ext->eflags |= MMC_EFLAG_HS;

	if (ext->eflags & MMC_EFLAG_FORCECLK){
		clock = ext->nclock;
	}else{
		clock = ext->csd.mmc_csd.ext_csd.hs_max_dtr;
		hs=1;
	}

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : config clock as %d", clock);

	ext->set_clock(hba, &clock, hs);

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : actual operation clock %d", clock);

	/*
	 * Activate wide bus (if supported).
	 */
	if ((ext->csd.mmc_csd.mmc_prot >=CSD_VERSION_14) &&
	    (ext->hccap & (MMC_HCCAP_BW8 | MMC_HCCAP_BW4))) {
		unsigned ext_csd_bit=EXT_CSD_BUS_WIDTH_1;
		if(ext->eflags & MMC_EFLAG_FORCEBW){
			if(ext->bus_width==8)
				ext_csd_bit = EXT_CSD_BUS_WIDTH_8;
			else if(ext->bus_width==4)
				ext_csd_bit = EXT_CSD_BUS_WIDTH_4;
		}else if (ext->hccap & MMC_HCCAP_BW8) {
			ext_csd_bit = EXT_CSD_BUS_WIDTH_8;
			ext->bus_width = 8;
		} else if(ext->hccap & MMC_HCCAP_BW4){
			ext_csd_bit = EXT_CSD_BUS_WIDTH_4;
			ext->bus_width = 4;
		}else{
			ext->bus_width = 1;
		}
		ext->cfg_bus(hba, ext->bus_width, 0);
		
		ext->ident = MMCSD_IDENT_MMC_SWITCH_MODE_BUSWIDTH;
		
		return mmc_sendcmd(hba, SD_SET_BUS_WIDTH, MMC_RSP_R1B, MMC_CMD_INTR, (0x3 << 24) |
		  (EXT_CSD_BUS_WIDTH << 16) | (ext_csd_bit << 8) | 0x1);

	}else{
		return mmc_ident_mmc_clock(hba);
	}
	return (CAM_FAILURE);
}

/*
 * Read and decode extended CSD.
 */
static int mmc_ident_read_ext_csd(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;
	uint8_t 		ext_csd[EXT_CSD_SIZE];
	int 			ext_csd_struct;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR){
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC :SEND_EXT_CSD command failed");
		return (CAM_FAILURE);
	}

	if (ext->pio_done(hba, (char *)ext_csd, EXT_CSD_SIZE, MMC_DIR_IN) != EXT_CSD_SIZE){
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC : pio read EXT_CSD failed");
		return (CAM_FAILURE);
	}

	ext_csd_struct = ext_csd[EXT_CSD_REV];
	if (ext_csd_struct >= 2) {
		ext->csd.mmc_csd.ext_csd.sectors =
			ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
			ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
			ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
			ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
		if(ext->csd.mmc_csd.ext_csd.sectors){
			atomic_set(&ext->eflags, MMC_EFLAG_HC);
			if (hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card : high capacity");
		}else{
			if(ext->eflags & MMC_EFLAG_HC){
				atomic_clr(&ext->eflags, MMC_EFLAG_HC);
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card : How come it is not high capacity");
			}
		}
	}

	switch (ext_csd[EXT_CSD_CARD_TYPE]) {
	case EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_26:
		ext->csd.mmc_csd.ext_csd.hs_max_dtr = 52000000;
		break;
	case EXT_CSD_CARD_TYPE_26:
		ext->csd.mmc_csd.ext_csd.hs_max_dtr = 26000000;
		break;
	default:
		/* MMC v4 spec says this cannot happen */
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s: card is mmc v4 but doesn't "
			"support any high-speed modes.\n", __func__);
		return (CAM_FAILURE);
	}
	if (hba->verbosity){
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card : EXT_CSD structure: ");
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " EXT_CSD_REV: %x", ext_csd_struct);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " Bus Width: %x", ext_csd[EXT_CSD_BUS_WIDTH]);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " MAX DTR: %d", ext->csd.mmc_csd.ext_csd.hs_max_dtr);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SECTORS: %d", ext->csd.mmc_csd.ext_csd.sectors);
	}

	/*
	 * Activate high speed (if supported)
	 */
	if ((ext->csd.mmc_csd.ext_csd.hs_max_dtr != 0) &&
		(ext->hccap & MMC_HCCAP_HS)) {
		if (hba->verbosity)
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC: Switch to high speed mode.");
		
		ext->ident = MMCSD_IDENT_MMC_SWITCH_MODE_HS;
		
		return mmc_sendcmd(hba, SD_SET_BUS_WIDTH, MMC_RSP_R1B, MMC_CMD_INTR, (0x3 << 24) |
		  (EXT_CSD_HS_TIMING << 16) | (1 << 8) | 0x1);
	}else{
		return mmc_ident_mmc_clock(hba);
	}
	return (CAM_FAILURE);
}

static int mmc_ident_status_done(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;
	int				bus_width;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	// TODO!
	// Set power here

	if (ext->version >= MMC_VERSION_1) {	// MMC card
		if(ext->csd.mmc_csd.mmc_prot >=CSD_VERSION_14){
			/* MMC V4 need to read ext_csd*/
			ext->cfg_bus(hba, 1, 1);		
			ext->set_blksz(hba, EXT_CSD_SIZE);	
			if (ext->setup_pio(hba, EXT_CSD_SIZE, MMC_DIR_IN) != EXT_CSD_SIZE)
				return (CAM_FAILURE);
			ext->ident = MMCSD_IDENT_EXT_CSD;

			return mmc_sendcmd(hba, MMC_SEND_EXT_CSD, MMC_RSP_R1, MMC_CMD_DATA_IN | MMC_CMD_DATA, 0);
		}else{
			return mmc_ident_mmc_clock(hba);
		}
	}
 
	/*
	 * The host controller should support at least 1 of them
	 */
	if (ext->eflags & MMC_EFLAG_FORCEBW)
		bus_width = ext->bus_width;
	else if (ext->hccap & MMC_HCCAP_BW4)
		bus_width = 4;		// 4 bit bus
	else
		bus_width = 1;		// 1 bit bus

	ext->cfg_bus(hba, bus_width, 0);

	if (bus_width == 4) {
		ext->ident = MMCSD_IDENT_SET_BW_APP;

		return mmc_sendcmd(hba, MMC_APP_CMD, MMC_RSP_R1, MMC_CMD_INTR, ext->rca << 16);
	}

	// Read SCR
	ext->ident = MMCSD_IDENT_SEND_SCR;

	return mmc_sendcmd(hba, MMC_APP_CMD, MMC_RSP_R1, MMC_CMD_INTR, ext->rca << 16);
}

static int mmc_ident_start(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	static int		count = 0;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (count == 0) {
		atomic_clr(&ext->eflags, MMC_EFLAG_WP | MMC_EFLAG_HS | MMC_EFLAG_HC);
		atomic_set(&ext->eflags, MMC_EFLAG_PPL);
	}

	if (ext->detect(hba) != MMC_SUCCESS)
		return -1;

	ext->powerdown(hba);

	delay(10);

	if (ext->powerup(hba) != MMC_SUCCESS)
		return (CAM_FAILURE);

	delay(10);

	if (++count > 3) {
		ext->ident = MMCSD_IDENT_GO_IDLE;
		count = 0;
	}

	return mmc_sendcmd(hba, MMC_GO_IDLE_STATE, MMC_RSP_NONE, MMC_CMD_INIT | MMC_CMD_INTR, 0);
}

static int mmc_ident_mmc_card_powerup(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;
	int				ocr=0;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (ext->istage > 300)
		return (MMC_FAILURE);

	ext->istage++;
	delay(10);
	if (ext->istage > 1) {
		if (intr & MMC_INTR_ERROR)
			return (MMC_FAILURE);

		/* High capacity card ? */
		if (ext->mmc_resp[0] & 0x40000000) {
			atomic_set(&ext->eflags, MMC_EFLAG_HC);
		}

		/* Power up ? */
		if (ext->mmc_resp[0] & 0x80000000) {
			ext->ident = MMCSD_IDENT_ALL_SEND_CID;	// card is powered up, go next
			ext->istage = 0;
			ext->version = MMC_VERSION_1;
			if (hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card detected");
			if ((ext->eflags & MMC_EFLAG_HC) && hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card : high capacity");

			return mmc_sendcmd(hba, MMC_ALL_SEND_CID, MMC_RSP_R2, MMC_CMD_INTR, 0);
		}
	}

	//tell the card that we support High Capcity
	if(ext->istage > 1 && (ext->hccap & MMC_HCCAP_HS))
		ocr = 0x40ff8000;
	else if(ext->istage > 1)
		ocr = 0x00ff8000;
	else ocr = 0;

	return mmc_sendcmd(hba, MMC_SEND_OP_COND, MMC_RSP_R3, MMC_CMD_INTR, ocr);
}

static int mmc_ident_sd_card_powerup(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;
	int				ocr;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (ext->istage > 1000) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "MMC card power up failed");
		return (MMC_FAILURE);
	}

	if ((ext->istage % 3) == 0) {
		delay(5);
		ext->istage++;
		return mmc_sendcmd(hba, MMC_APP_CMD, MMC_RSP_R1, MMC_CMD_INTR, 0);
	}

	ocr = 0x00ff8000;

	if (ext->istage == 1) {
		if (ext->version == SD_VERSION_1 && (intr & MMC_ERR_CMD_TO)) {
			// timeout, might be a MMC card
			ext->ident = MMCSD_IDENT_MMC_POWERUP;
			ext->istage = 0;
			ext->eflags &= ~MMC_EFLAG_PPL;
#if 1
			return mmc_sendcmd(hba, MMC_GO_IDLE_STATE, MMC_RSP_NONE, MMC_CMD_INIT | MMC_CMD_INTR, 0);
#else
			return mmc_ident_mmc_card_powerup(hba, intr);
#endif
		} else if (intr & MMC_INTR_ERROR) {
			return (MMC_FAILURE);
		}
		ocr = 0;
	}

	switch (ext->istage % 3) {
		case 1:		// wait for MMC_APP_CMD complete
			if (intr & MMC_INTR_ERROR)
				return (MMC_FAILURE);
			if (ext->version == SD_VERSION_2)
				ocr |= 1 << 30;
			ext->istage++;
			return mmc_sendcmd(hba, SD_SEND_OP_COND, MMC_RSP_R3, MMC_CMD_INTR, ocr);

		case 2:		// wait for SD_SEND_OP_COND complete
			if (intr & MMC_INTR_ERROR)
				return (MMC_FAILURE);

			/* High capacity card ? */
			if (ext->mmc_resp[0] & 0x40000000) {
				atomic_set(&ext->eflags, MMC_EFLAG_HC);
			}

			/* Power up ? */
			if (ext->mmc_resp[0] & 0x80000000) {
				ext->ident = MMCSD_IDENT_ALL_SEND_CID;	// card is powered up, go next
				ext->istage = 0;

				if ((ext->eflags & MMC_EFLAG_HC) && hba->verbosity)
					slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD card : high capacity");

				if (hba->verbosity)
					slogf(_SLOGC_SIM_MMC, _SLOG_INFO,
						"SD card version = %d", ext->version);


				return mmc_sendcmd(hba, MMC_ALL_SEND_CID, MMC_RSP_R2, MMC_CMD_INTR, 0);
			} else
				ext->istage++;

			return mmc_sendcmd(hba, MMC_APP_CMD, MMC_RSP_R1, MMC_CMD_INTR, 0);
	}

	return (MMC_FAILURE);
}

static int mmc_ident_go_idle(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext;

	ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (MMC_FAILURE);

	if (intr & MMC_INTR_COMMAND) {
		ext->ident = MMCSD_IDENT_CARD_VERSION;
		ext->istage = 0;

		delay(10);

		return mmc_sendcmd(hba, MMC_IF_COND, MMC_RSP_R7, MMC_CMD_INTR, 0x1AA);
	}

	return (MMC_SUCCESS);
}

static int mmc_ident_card_version(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_ERR_CMD_TO)			// timeout
		ext->version = SD_VERSION_1;
	else if (!(intr & MMC_INTR_ERROR) && (ext->mmc_resp[0] & 0x1FFF) == 0x1AA)
		ext->version = SD_VERSION_2;	// Version 2.0
	else
		return (MMC_FAILURE);

	delay(10);

	ext->ident = MMCSD_IDENT_SD_POWERUP;		// goto the next stage
	ext->istage = 0;

	return mmc_ident_sd_card_powerup(hba, intr);
}

static int mmc_ident_all_send_cid(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);
	ext->ident = MMCSD_IDENT_SET_ADDRESS;
	return mmc_sendcmd(hba, MMC_SET_RELATIVE_ADDR, MMC_RSP_R1, MMC_CMD_INTR, 0);
}

static int mmc_ident_set_address(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	ext->eflags |= MMC_EFLAG_PPL;
	ext->rca     = ext->mmc_resp[0] >> 16;
	ext->ident   = MMCSD_IDENT_SEND_CID;

	/* Ask card to send CID */
	return mmc_sendcmd(hba, MMC_SEND_CID, MMC_RSP_R2, MMC_CMD_INTR, ext->rca << 16);
}

static int mmc_ident_send_cid(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);
	ext->raw_cid[0] = ext->mmc_resp[0];
	ext->raw_cid[1] = ext->mmc_resp[1];
	ext->raw_cid[2] = ext->mmc_resp[2];
	ext->raw_cid[3] = ext->mmc_resp[3];
	ext->ident = MMCSD_IDENT_SEND_CSD;

	return mmc_sendcmd(hba, MMC_SEND_CSD, MMC_RSP_R2, MMC_CMD_INTR, ext->rca << 16);
}

static int mmc_ident_send_csd(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);
	ext->raw_csd[0] = ext->mmc_resp[0];
	ext->raw_csd[1] = ext->mmc_resp[1];
	ext->raw_csd[2] = ext->mmc_resp[2];
	ext->raw_csd[3] = ext->mmc_resp[3];
	if (hba->verbosity) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "CSD : %08X:%08X:%08X:%08X", 
			ext->raw_csd[0], ext->raw_csd[1], ext->raw_csd[2], ext->raw_csd[3]);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "CID : %08X:%08X:%08X:%08X", 
			ext->raw_cid[0], ext->raw_cid[1], ext->raw_cid[2], ext->raw_cid[3]);
	}
	if (ext->version < MMC_VERSION_1) {
		mmc_parse_sd_csd(hba, &ext->csd.sd_csd, ext->raw_csd);
		mmc_parse_sd_cid(hba, &ext->cid.sd_cid, ext->raw_cid);
	} else {
		mmc_parse_mmc_csd(hba, &ext->csd.mmc_csd, ext->raw_csd);
		mmc_parse_mmc_cid(hba, &ext->cid.mmc_cid, ext->raw_cid);
	}

	ext->ident = MMCSD_IDENT_SEL_DES_CARD;
	return mmc_sendcmd(hba, MMC_SEL_DES_CARD, MMC_RSP_R1, MMC_CMD_INTR, ext->rca << 16);
}

static int mmc_ident_send_scr(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	if (ext->setup_pio(hba, SD_SCR_SIZE, MMC_DIR_IN) != 8)
		return (CAM_FAILURE);

	ext->ident = MMCSD_IDENT_PARSE_SCR;

	return mmc_sendcmd(hba, SD_SEND_SCR, MMC_RSP_R1, MMC_CMD_DATA_IN | MMC_CMD_DATA, 0);
}

static int mmc_ident_parse_scr(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
	uint8_t			*pscr = &ext->raw_scr[0];

	if (intr & MMC_INTR_ERROR) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: read SCR failed, skip high speed probe.");
		return (CAM_FAILURE);
	}

	if (ext->pio_done(hba, (char *)pscr, SD_SCR_SIZE, MMC_DIR_IN) != 8)
		return (CAM_FAILURE);

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SCR : %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", 
			pscr[0], pscr[1], pscr[2], pscr[3], pscr[4], pscr[5], pscr[6], pscr[7]);

	ext->sd_scr.scr_structure         = pscr[0] >> 4;
	ext->sd_scr.sd_spec               = pscr[0] & 0x0F;
	ext->sd_scr.data_stat_after_erase = pscr[1] >> 7;
	ext->sd_scr.sd_security           = (pscr[1] >> 4) & 0x0F;
	ext->sd_scr.sd_bus_width          = pscr[1] & 0x0F;

	if (hba->verbosity) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD SCR info:");
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SCR_STRUCTURE         : %x", ext->sd_scr.scr_structure);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SD_SPEC               : %x", ext->sd_scr.sd_spec);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " DATA_STAT_AFTER_ERASE : %x", ext->sd_scr.data_stat_after_erase);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SD_SECURITY           : %x", ext->sd_scr.sd_security);
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, " SD_BUS_WIDTH          : %x", ext->sd_scr.sd_bus_width);
	}

	if (ext->sd_scr.sd_spec < 1 || ext->set_blksz(hba, 64) != MMC_SUCCESS)
		return mmc_ident_sd_clock(hba);

	if (ext->setup_pio(hba, 64, MMC_DIR_IN) != 64)
		return mmc_ident_sd_clock(hba);

	ext->ident = MMCSD_IDENT_CHECK_MODE;

	return mmc_sendcmd(hba, SD_SET_BUS_WIDTH, MMC_RSP_R1, MMC_CMD_DATA_IN | MMC_CMD_DATA, 0x00ffff01);
}


static int mmc_ident_check_mode(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
	uint8_t			tmp[64];

	if (intr & MMC_INTR_ERROR) {
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: check mode failed, skip high speed probe.");
		return mmc_ident_sd_clock(hba);
	}

	if (ext->pio_done(hba, (char *)tmp, 64, MMC_DIR_IN) != 64){
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: check mode pio read failed, skip high speed probe.");
		return (CAM_FAILURE);
	}

	if ((tmp[13] & 0x02) == 0)
		goto nohs;

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: Card support high speed mode.");

	if (!(ext->csd.sd_csd.ccc & (1 << 10)))	// Switch command supported?
		goto nohs;

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: Card support switch command.");

	if (!(ext->hccap & MMC_HCCAP_HS))	// Host support HS?
		goto nohs;

	ext->max_dtr = 50000000;

	if (ext->setup_pio(hba, 64, MMC_DIR_IN) != 64)
		return (CAM_FAILURE);

	if (hba->verbosity)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: Switch to high speed mode.");

	ext->ident = MMCSD_IDENT_SWITCH_MODE;

	return mmc_sendcmd(hba, SD_SET_BUS_WIDTH, MMC_RSP_R1, MMC_CMD_DATA_IN | MMC_CMD_DATA, 0x80ffff01);

nohs:
	return mmc_ident_sd_clock(hba);
}

static int mmc_ident_switch_mode(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;
	uint8_t			tmp[64];

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	if (ext->pio_done(hba, (char *)tmp, 64, MMC_DIR_IN) != 64)
		return (CAM_FAILURE);

	if ((tmp[16] & 0x0F) == 1){
		ext->eflags |= MMC_EFLAG_HS;
		if (hba->verbosity)
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "SD: Switch to high speed mode successful");
	}
	return mmc_ident_sd_clock(hba);
}

static int mmc_ident_sel_des_card(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	ext->ident = MMCSD_IDENT_CHECK_STS;

	return mmc_sendcmd(hba, MMC_SEND_STATUS, MMC_RSP_R1, MMC_CMD_INTR, ext->rca << 16);
}

static int mmc_ident_setbw_app(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	ext->ident = MMCSD_IDENT_SET_BW;

	return mmc_sendcmd(hba, SD_SET_BUS_WIDTH, MMC_RSP_R1, MMC_CMD_INTR, 2);
}

static int mmc_ident_setbw(SIM_HBA *hba, int intr)
{
	SIM_MMC_EXT		*ext = (SIM_MMC_EXT *)hba->ext;

	if (intr & MMC_INTR_ERROR)
		return (CAM_FAILURE);

	if ((ext->eflags & MMC_EFLAG_NOHS) || !ext->setup_pio || ext->set_blksz(hba, 8) != MMC_SUCCESS)
		return mmc_ident_sd_clock(hba);

	// Read SCR
	ext->ident = MMCSD_IDENT_SEND_SCR;

	return mmc_sendcmd(hba, MMC_APP_CMD, MMC_RSP_R1, MMC_CMD_INTR, ext->rca << 16);
}

int mmc_card_ident(SIM_HBA *hba, uint32_t intr)
{
	SIM_MMC_EXT		*ext;

	ext = (SIM_MMC_EXT *)hba->ext;

#ifdef MMCSD_DEBUG
	mmc_slogf( hba, _SLOGC_SIM_MMC, _SLOG_ERROR, 1, "%s: ident %s", __FUNCTION__, mmc_idents[ext->ident] );
#endif

	switch (ext->ident) {
		case MMCSD_IDENT_START:
			return mmc_ident_start(hba);

		case MMCSD_IDENT_GO_IDLE:
			return mmc_ident_go_idle(hba, intr);

		case MMCSD_IDENT_CARD_VERSION:
			return mmc_ident_card_version(hba, intr);

		case MMCSD_IDENT_SD_POWERUP:
			return mmc_ident_sd_card_powerup(hba, intr);

		case MMCSD_IDENT_MMC_POWERUP:
			return mmc_ident_mmc_card_powerup(hba, intr);

		case MMCSD_IDENT_ALL_SEND_CID:
			return mmc_ident_all_send_cid(hba, intr);

		case MMCSD_IDENT_SET_ADDRESS:
			return mmc_ident_set_address(hba, intr);

		case MMCSD_IDENT_SEND_CID:
			return mmc_ident_send_cid(hba, intr);

		case MMCSD_IDENT_SEND_CSD:
			return mmc_ident_send_csd(hba, intr);

		case MMCSD_IDENT_SEL_DES_CARD:
			return mmc_ident_sel_des_card(hba, intr);

		case MMCSD_IDENT_SEND_SCR:
			return mmc_ident_send_scr(hba, intr);

		case MMCSD_IDENT_PARSE_SCR:
			return mmc_ident_parse_scr(hba, intr);

		case MMCSD_IDENT_CHECK_MODE:
			return mmc_ident_check_mode(hba, intr);

		case MMCSD_IDENT_SWITCH_MODE:
			return mmc_ident_switch_mode(hba, intr);

		case MMCSD_IDENT_CHECK_STS:
			return mmc_ident_status_done(hba, intr);

		case MMCSD_IDENT_SET_BW_APP:
			return mmc_ident_setbw_app(hba, intr);

		case MMCSD_IDENT_SET_BW:
			return mmc_ident_setbw(hba, intr);

		case MMCSD_IDENT_EXT_CSD:
			return mmc_ident_read_ext_csd(hba, intr);

		case MMCSD_IDENT_MMC_SWITCH_MODE_HS:
			return mmc_ident_switch_mode_hs(hba, intr);

		case MMCSD_IDENT_MMC_SWITCH_MODE_BUSWIDTH:
			return mmc_ident_switch_buswidth(hba, intr);

		case MMCSD_IDENT_FINAL:
			ext->ident = MMCSD_IDENT_NONE;
			if (intr & MMC_INTR_ERROR)
				return (CAM_FAILURE);
			break;
		default:
			return (CAM_FAILURE);
	}

	return (CAM_SUCCESS);
}

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devb/mmcsd/mmcsd.c $ $Rev: 249398 $" );

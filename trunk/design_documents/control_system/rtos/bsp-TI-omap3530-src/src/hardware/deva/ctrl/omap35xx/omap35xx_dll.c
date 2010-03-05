/*
 * $QNXLicenseC:
 * Copyright 2006-2008 QNX Software Systems.
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

#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>
#include "omap35xx.h"

int     codec_mixer (ado_card_t * card, HW_CONTEXT_T * omap35xx);

int32_t
omap35xx_capabilities (HW_CONTEXT_T * omap35xx, snd_pcm_channel_info_t * info)
{
	int     chn_avail = 1;

	if (info->channel == SND_PCM_CHANNEL_PLAYBACK && omap35xx->play_strm.pcm_subchn)
		chn_avail = 0;
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE && omap35xx->cap_strm.pcm_subchn)
		chn_avail = 0;

	if (chn_avail == 0)
	{
		info->formats = 0;
		info->rates = 0;
		info->min_rate = 0;
		info->max_rate = 0;
		info->min_voices = 0;
		info->max_voices = 0;
		info->min_fragment_size = 0;
		info->max_fragment_size = 0;
	}
	return 0;
}

static void
omap35xx_frag_dmabuf_sizer (omap35xx_t * omap35xx, int channel, ado_pcm_config_t * config,
	ado_pcm_subchn_t * subchn)
{
	int32_t result, frag_size;
	int     voices =
		channel ==
		SND_PCM_CHANNEL_PLAYBACK ? omap35xx->play_strm.voices : omap35xx->cap_strm.voices;

	frag_size = ado_pcm_dma_int_size (config);
	/* The overall DMA buffer size seems to need to be a multiple of 64 or the DMA engine gets stuck when
	 * it loops back on itself (i.e. transfer complete). Also take into account the number of voices
	 * to ensure voice conversion aligns with the fragment boundary.
	 */
	result = (frag_size / (voices * 64)) * (voices * 64);
	if (config->trans_mode == SND_PCM_MODE_BLOCK)
	{
		config->mode.block.frag_size = result;
		config->dmabuf.size = config->mode.block.frags_total * config->mode.block.frag_size;
	}
	else
	{
		config->mode.stream.queue_size = result;
		config->dmabuf.size = config->mode.stream.queue_size;
	}
}

int32_t
omap35xx_playback_aquire (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	int     idx = omap35xx->play_strm.dma_chan_idx;

	if (omap35xx->play_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return EAGAIN;
	}

	ado_mutex_lock (&omap35xx->hw_lock);

	omap35xx_frag_dmabuf_sizer (omap35xx, SND_PCM_CHANNEL_PLAYBACK, config, subchn);
	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&omap35xx->hw_lock);
		return errno;
	}

	/* Set the element size to 16 bits */
	omap35xx->dma4->channel[idx].csdp = DMA4_CSDP_DATA_TYPE_16BIT;
	omap35xx->dma4->channel[idx].ccr |= (DMA4_CCR_SRC_POST_INCR | DMA4_CCR_DST_CONSTANT);

	omap35xx->dma4->channel[idx].cen = config->mode.block.frag_size / 2;	/* samples per frag/frame */
	omap35xx->dma4->channel[idx].cfn = config->mode.block.frags_total;

	/* Set the Physical Address for the source-side of the DMA channel */
	omap35xx->dma4->channel[idx].cssa = config->dmabuf.phys_addr;

	/* Set the Physical Address for the destination-side of the DMA channel */
	omap35xx->dma4->channel[idx].cdsa = omap35xx->mcbsp_addr + offsetof (mcbsp_t, mcbsplp_dxr_reg);

	/* Clear previous status of the channel */
	omap35xx->dma4->channel[idx].csr = DMA4_CSR_MSK;

	/* Enable end of frame/fragment interrupt */
	omap35xx->dma4->channel[idx].cicr = DMA4_CICR_FRAME_IE;

	/* Chain (or loop-back) the DMA channel to it's self */
	omap35xx->dma4->channel[idx].clnk_ctrl = DMA4_CLNK_CTRL_ENABLE_LNK | idx;

	omap35xx->play_strm.pcm_subchn = subchn;
	*pc = (PCM_SUBCHN_CONTEXT_T *) omap35xx->play_strm.pcm_subchn;
	omap35xx->play_strm.pcm_config = config;
	ado_mutex_unlock (&omap35xx->hw_lock);
	return EOK;
}

int32_t
omap35xx_playback_release (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&omap35xx->hw_lock);
	omap35xx->play_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&omap35xx->hw_lock);
	return 0;
}

int32_t
omap35xx_capture_aquire (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	int     idx = omap35xx->cap_strm.dma_chan_idx;

	if (omap35xx->cap_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return EAGAIN;
	}

	ado_mutex_lock (&omap35xx->hw_lock);

	omap35xx_frag_dmabuf_sizer (omap35xx, SND_PCM_CHANNEL_CAPTURE, config, subchn);
	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&omap35xx->hw_lock);
		return errno;
	}

	/* Set the element size to 16 bits */
	omap35xx->dma4->channel[idx].csdp = DMA4_CSDP_DATA_TYPE_16BIT;
	omap35xx->dma4->channel[idx].ccr |= DMA4_CCR_SRC_CONSTANT | DMA4_CCR_DST_POST_INCR;

	omap35xx->dma4->channel[idx].cen = config->mode.block.frag_size / 2;	/* samples per frag/frame */
	omap35xx->dma4->channel[idx].cfn = config->mode.block.frags_total;

	/* Set the Physical Address for the source-side of the DMA channel */
	omap35xx->dma4->channel[idx].cssa = omap35xx->mcbsp_addr + offsetof (mcbsp_t, mcbsplp_drr_reg);

	/* Set the Physical Address for the destination-side of the DMA channel */
	omap35xx->dma4->channel[idx].cdsa = config->dmabuf.phys_addr;

	/* Clear previous status of the channel */
	omap35xx->dma4->channel[idx].csr = DMA4_CSR_MSK;

	/* Enable end of frame/fragment interrupt */
	omap35xx->dma4->channel[idx].cicr = DMA4_CICR_FRAME_IE;

	/* Chain (or loop-back) the DMA channel to it's self */
	omap35xx->dma4->channel[idx].clnk_ctrl = DMA4_CLNK_CTRL_ENABLE_LNK | idx;

	omap35xx->cap_strm.pcm_subchn = subchn;
	*pc = (PCM_SUBCHN_CONTEXT_T *) omap35xx->cap_strm.pcm_subchn;
	omap35xx->cap_strm.pcm_config = config;
	ado_mutex_unlock (&omap35xx->hw_lock);
	return EOK;
}

int32_t
omap35xx_capture_release (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&omap35xx->hw_lock);
	omap35xx->cap_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&omap35xx->hw_lock);
	return 0;
}

int32_t
omap35xx_prepare (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	return 0;
}

int32_t
omap35xx_playback_trigger (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int     idx = omap35xx->play_strm.dma_chan_idx;

	ado_mutex_lock (&omap35xx->hw_lock);
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == omap35xx->play_strm.pcm_subchn)
		{
			omap35xx->dma4->channel[idx].ccr |= DMA4_CCR_ENABLE;
			/* Synchronize enable of transmitter with Frame sync to avoid channel hopping */
			omap35xx->mcbsp->mcbsplp_irqstatus_reg |= MCBSPLP_IRQSTATUS_REG_XFSX;
			while (!(omap35xx->mcbsp->mcbsplp_irqstatus_reg & MCBSPLP_IRQSTATUS_REG_XFSX));
			omap35xx->mcbsp->mcbsplp_xccr_reg &= ~MCBSPLP_XCCR_REG_XDISABLE;
		}
	}
	else
	{
		if (pc == omap35xx->play_strm.pcm_subchn)
		{
			int     cnt = 0;

			omap35xx->dma4->channel[idx].ccr &= ~DMA4_CCR_ENABLE;
			/* Wait for transmitter to empty and disable */
			while ((omap35xx->mcbsp->mcbsplp_xbuffstat_reg & 0xff) < 128 && cnt++ < 100000);
			omap35xx->mcbsp->mcbsplp_xccr_reg |= MCBSPLP_XCCR_REG_XDISABLE;

			/* Toggle transmitter reset to bring sanity to the McBSP (to avoid channel hopping) */
			omap35xx->mcbsp->mcbsplp_spcr2_reg &= ~MCBSPLP_SPCR2_REG_XRST;
			nanospin_ns (1000);
			omap35xx->mcbsp->mcbsplp_spcr2_reg |= MCBSPLP_SPCR2_REG_XRST;
		}
	}
	ado_mutex_unlock (&omap35xx->hw_lock);
	return 0;
}

int32_t
omap35xx_capture_trigger (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int     idx = omap35xx->cap_strm.dma_chan_idx;

	ado_mutex_lock (&omap35xx->hw_lock);
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == omap35xx->cap_strm.pcm_subchn)
		{
			omap35xx->dma4->channel[idx].ccr |= DMA4_CCR_ENABLE;

			/* Synchronize enable of receiver with Frame sync to avoid channel hopping */
			omap35xx->mcbsp->mcbsplp_irqstatus_reg |= MCBSPLP_IRQSTATUS_REG_RFSR;
			while (!(omap35xx->mcbsp->mcbsplp_irqstatus_reg & MCBSPLP_IRQSTATUS_REG_RFSR));
			omap35xx->mcbsp->mcbsplp_rccr_reg &= ~MCBSPLP_RCCR_REG_RDISABLE;
		}
	}
	else
	{
		if (pc == omap35xx->cap_strm.pcm_subchn)
		{
			/* Disable the McBSP Receiver at end of frame */
			omap35xx->mcbsp->mcbsplp_rccr_reg |= MCBSPLP_RCCR_REG_RDISABLE;
			omap35xx->dma4->channel[idx].ccr &= ~DMA4_CCR_ENABLE;

			/* Toggle receiver reset to bring sanity to the McBSP (to avoid channel hopping) */
			omap35xx->mcbsp->mcbsplp_spcr1_reg &= ~MCBSPLP_SPCR1_REG_RRST;
			nanospin_ns (1000);
			omap35xx->mcbsp->mcbsplp_spcr1_reg |= MCBSPLP_SPCR1_REG_RRST;

		}
	}
	ado_mutex_unlock (&omap35xx->hw_lock);
	return 0;
}

uint32_t
omap35xx_position (HW_CONTEXT_T * omap35xx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	uint32_t pos;
	int     idx = omap35xx->play_strm.dma_chan_idx;

	/* Position relative to current fragment */
	ado_mutex_lock (&omap35xx->hw_lock);
	if (pc == omap35xx->cap_strm.pcm_subchn)
	{
		idx = omap35xx->cap_strm.dma_chan_idx;
	}

	/* Fragsize - (Element idx * element size) -> (element == sample) */
	pos = ado_pcm_dma_int_size (config) - ((omap35xx->dma4->channel[idx].ccen & 0x00ffffff) * 2);
	ado_mutex_unlock (&omap35xx->hw_lock);

	return (pos);
}

void
omap35xx_dma_interrupt (HW_CONTEXT_T * omap35xx, int32_t irq)
{
	int     play_idx = omap35xx->play_strm.dma_chan_idx;
	int     cap_idx = omap35xx->cap_strm.dma_chan_idx;
	int     play_status = -1;
	int     cap_status = -1;


	ado_mutex_lock (&omap35xx->hw_lock);
	if (omap35xx->dma4->channel[play_idx].csr & DMA4_CSR_FRAME)
	{
		omap35xx->dma4->channel[play_idx].csr |= DMA4_CSR_FRAME;	/* Clear Interrupt status */
		play_status = play_idx;
	}

	if (omap35xx->dma4->channel[cap_idx].csr & DMA4_CSR_FRAME)
	{
		omap35xx->dma4->channel[cap_idx].csr |= DMA4_CSR_FRAME;	/* Clear Interrupt status */
		cap_status = cap_idx;
	}

	ado_mutex_unlock (&omap35xx->hw_lock);

	if (play_status == play_idx)
		dma_interrupt (omap35xx->play_strm.pcm_subchn);
	if (cap_status == cap_idx)
		dma_interrupt (omap35xx->cap_strm.pcm_subchn);
}

void
omap35xx_dma_detach (HW_CONTEXT_T * omap35xx)
{
	rsrc_request_t req = { 0 };

	if (omap35xx->play_strm.dma_chan_idx != -1)
	{
		req.length = 1;
		req.start = omap35xx->play_strm.dma_chan_idx;
		req.end = omap35xx->play_strm.dma_chan_idx;
		req.flags = RSRCDBMGR_DMA_CHANNEL;
		if (rsrcdbmgr_detach (&req, 1) == -1)
		{
			ado_error ("Unable to detach DMA playback channel (%s) \n", strerror (errno));
		}
	}
	if (omap35xx->cap_strm.dma_chan_idx != -1)
	{
		memset (&req, 0, sizeof (req));
		req.length = 1;
		req.start = omap35xx->cap_strm.dma_chan_idx;
		req.end = omap35xx->cap_strm.dma_chan_idx;
		req.flags = RSRCDBMGR_DMA_CHANNEL;
		if (rsrcdbmgr_detach (&req, 1) == -1)
		{
			ado_error ("Unable to detach DMA capture channel (%s) \n", strerror (errno));
		}
	}
	munmap_device_memory (omap35xx->dma4, sizeof (dma4_t));
}

int32_t
omap35xx_dma_attach (HW_CONTEXT_T * omap35xx, ado_card_t * card)
{
	int     play_idx = omap35xx->play_strm.dma_chan_idx = -1;
	int     cap_idx = omap35xx->cap_strm.dma_chan_idx = -1;
	rsrc_request_t req = { 0 };

	/* Map in DMA registers */
	if ((omap35xx->dma4 = mmap_device_memory (0, sizeof (dma4_t),
				PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, DMA4_BASE_ADDR)) == MAP_FAILED)
	{
		ado_error ("Unable to mmap DMA (%s) \n", strerror (errno));
		return (-1);
	}
	omap35xx->dma4->gcr = GCR_ARBITRATION_RATE | DMA_DEFAULT_FIFO_DEPTH;

	if (MCBSP_PLAYBACK_PATH == ENABLED)
	{
		req.length = 1;
		req.flags = RSRCDBMGR_DMA_CHANNEL;
		if (rsrcdbmgr_attach (&req, 1) == -1)
		{
			ado_error ("Unable to aquire DMA channel for playback (%s) \n", strerror (errno));
			munmap_device_memory (omap35xx->dma4, sizeof (dma4_t));
			return (-1);
		}
		play_idx = omap35xx->play_strm.dma_chan_idx = req.start;

		/* Config DMA Channel for Playback DMA request */
		omap35xx->dma4->channel[play_idx].ccr =
			((omap35xx->mcbsp_tx_dma_req + 1) & 0x1f) | (((omap35xx->mcbsp_tx_dma_req +
					1) & 0x60) << 14);
		omap35xx->dma4->channel[play_idx].ccr |= (DMA4_CCR_FS | DMA4_CCR_BS);	/* Packet transfer Mode */
		omap35xx->dma4->channel[play_idx].cdf = 16;		   /* Packet size (MCBSP_FIFO_THRESHOLD + 1) */

		if (ado_attach_interrupt (card, (SDMA_BASE_IRQ + play_idx), omap35xx_dma_interrupt,
				omap35xx) != 0)
		{
			ado_error ("Unable to attach play interrupt (%s)", strerror (errno));
			omap35xx_dma_detach (omap35xx);
			return -1;
		}

	}
	if (MCBSP_CAPTURE_PATH == ENABLED)
	{
		memset (&req, 0, sizeof (req));
		req.length = 1;
		req.flags = RSRCDBMGR_DMA_CHANNEL;
		if (rsrcdbmgr_attach (&req, 1) == -1)
		{
			ado_error ("Unable to aquire DMA channel for capture (%s) \n", strerror (errno));
			omap35xx_dma_detach (omap35xx);
			return (-1);
		}
		cap_idx = omap35xx->cap_strm.dma_chan_idx = req.start;

		/* Config DMA Channel for Capture DMA request */
		omap35xx->dma4->channel[cap_idx].ccr =
			((omap35xx->mcbsp_rx_dma_req + 1) & 0x1f) | (((omap35xx->mcbsp_rx_dma_req +
					1) & 0x60) << 14);
		omap35xx->dma4->channel[cap_idx].ccr |= (DMA4_CCR_FS | DMA4_CCR_BS);	/* Packet transfer Mode */
		omap35xx->dma4->channel[cap_idx].ccr |= DMA4_CCR_SEL_SRC_DST_SYNC;	/* Transfer is trigger by the Source (HW) */
		omap35xx->dma4->channel[cap_idx].csf = 16;		   /* Packet size (MCBSP_FIFO_THRESHOLD + 1) */

		if (ado_attach_interrupt (card, (SDMA_BASE_IRQ + cap_idx), omap35xx_dma_interrupt,
				omap35xx) != 0)
		{
			ado_error ("Unable to attach cap interrupt (%s)", strerror (errno));
			omap35xx_dma_detach (omap35xx);
			return -1;
		}
	}
	return 0;
}

int32_t
mcbsp_init (HW_CONTEXT_T * omap35xx)
{
	int     divider, frame_width;
	mcbsp_t *mcbsp = omap35xx->mcbsp;

	ado_mutex_lock (&omap35xx->hw_lock);

	/* Disable all MCBSP interrupts */
	omap35xx->mcbsp->mcbsplp_irqenable_reg = 0x0;

	/* Put MCBSP into reset */
	mcbsp->mcbsplp_spcr2_reg = 0x0;						   /* TX */
	mcbsp->mcbsplp_spcr1_reg = 0x0;						   /* RX */

	if (omap35xx->clk_mode == MASTER)
	{
		/* Config MCBSP as Master -> BitClks (CLKX/CLKR) are outputs, FrameSyncs (FSXM/FSRM) are outputs. */
		mcbsp->mcbsplp_pcr_reg =
			MCBSPLP_PCR_REG_FSXM | MCBSPLP_PCR_REG_CLKXM | MCBSPLP_PCR_REG_FSRM |
			MCBSPLP_PCR_REG_CLKRM;
	}
	else
	{
		/* Config MCBSP as Slave -> BitClks (CLKX/CLKR) are inputs, FrameSyncs (FSXM/FSRM) are inputs. */
		mcbsp->mcbsplp_pcr_reg &=
			~(MCBSPLP_PCR_REG_FSXM | MCBSPLP_PCR_REG_CLKXM | MCBSPLP_PCR_REG_FSRM |
			MCBSPLP_PCR_REG_CLKRM);
	}

#if defined (CODEC_FORMAT_TDM)

	/* Transmit & Receive on rising edge, Frame Sync active high */
	mcbsp->mcbsplp_pcr_reg |= MCBSPLP_PCR_REG_CLKRP;

	/* The McBSP does not seem to have any notion of a TDM slot, all it knows is word size.
	 * So in order to transmit/receive a 16bit word in a 32bit TDM slot we must configure
	 * the McBSP for 2x the number of words per frame and only transmite/receive data in the 
	 * MSB byte of each slot (even numbered words/channels).
	 */

	/* TX: Enable 8-partition Mode, All channels disabled by default */
	mcbsp->mcbsplp_mcr2_reg = MCBSPLP_MCR2_REG_XMCME | MCBSPLP_MCR2_REG_XMCM;
	/* TX: Single Phase (TDM), 16-bit words, 1bit delay */
	mcbsp->mcbsplp_xcr2_reg = MCBSPLP_XCR_XDATDLY_1;
	mcbsp->mcbsplp_xcr1_reg = MCBSPLP_XCR_XWDLEN_16;

	switch (omap35xx->play_strm.voices)
	{
	case 1:
		mcbsp->mcbsplp_xcera_reg = 0x01;				   /* Enable channels 0 (Mono) */
		break;
	case 2:
	default:
		/* TX: 2x2 words per frame */
		mcbsp->mcbsplp_xcr1_reg |= MCBSPLP_XCR_XFRLEN_4;
		mcbsp->mcbsplp_xcera_reg = 0x05;				   /* Enable channels 0 & 2 (stereo) */
		break;
	case 3:
		/* TX: 2x3 words per frame */
		mcbsp->mcbsplp_xcr1_reg |= MCBSPLP_XCR_XFRLEN_6;
		mcbsp->mcbsplp_xcera_reg = 0x15;				   /* Enable channels 0, 2 & 4 (3chn) */
		break;

	case 4:
		/* TX: 2x4 words per frame */
		mcbsp->mcbsplp_xcr1_reg |= MCBSPLP_XCR_XFRLEN_8;
		mcbsp->mcbsplp_xcera_reg = 0x55;				   /* Enable channels 0, 2, 4 & 6 for transmit (4chn) */
		break;
	}

	/* RX: Enable 8-partition Mode, All channels disabled by default */
	mcbsp->mcbsplp_mcr1_reg = MCBSPLP_MCR1_REG_RMCME | MCBSPLP_MCR1_REG_RMCM;
	/* RX: Single Phase (TDM), 16-bit words, 1bit delay */
	mcbsp->mcbsplp_rcr2_reg = MCBSPLP_RCR_RDATDLY_1;
	mcbsp->mcbsplp_rcr1_reg = MCBSPLP_RCR_RWDLEN_16;

	switch (omap35xx->cap_strm.voices)
	{
	case 1:
		mcbsp->mcbsplp_rcera_reg = 0x01;				   /* Enable channel 0 (Mono) */
		break;
	case 2:
	default:
		/* RX: 2x2 words per frame */
		mcbsp->mcbsplp_rcr1_reg |= MCBSPLP_RCR_RFRLEN_4;
		mcbsp->mcbsplp_rcera_reg = 0x05;				   /* Enable channels 0 & 2 (stereo) */
		break;
	}

	/* Calculate the CLK divider and the FS pulse width. 
	 * Program 1 less the desired value as per the docs. 
	 */
	frame_width = SLOT_SIZE * NSLOTS;					   /* frame_width = slot size * number of slots */
	divider = OMAP35XX_MASTER_CLK / (OMAP35XX_FRAME_RATE * frame_width);
	mcbsp->mcbsplp_srgr1_reg = (divider - 1) | ((SLOT_SIZE - 1) << 8);

#else

	if ((omap35xx->play_strm.voices != omap35xx->cap_strm.voices) ||
		(omap35xx->play_strm.voices > 2))
	{
		ado_error ("Invalid voice configuration");
		return -1;
	}

#if (defined CODEC_FORMAT_PCM)
	/* Transmit driven on rising edge,
	 * Receive sampled on the falling edge,
	 * Frame Sync polarity active high
	 */
	mcbsp->mcbsplp_pcr_reg |= MCBSPLP_PCR_REG_CLKRP;
#else						 /* Default to I2S Format */
	/* Transmit driven on falling edge,
	 * Receive sampled on the falling edge,
	 * Frame Sync polarity active high
	 */
	mcbsp->mcbsplp_pcr_reg |= MCBSPLP_PCR_REG_CLKXP | MCBSPLP_PCR_REG_CLKRP;
#endif

	/* Disable transmit multi-channel mode */
	mcbsp->mcbsplp_mcr2_reg = 0;
	mcbsp->mcbsplp_mcr1_reg = 0;

#if (defined CODEC_FORMAT_PCM)
	/* 0-bit delay for PCM format */
	mcbsp->mcbsplp_xcr2_reg = 0;
	mcbsp->mcbsplp_rcr2_reg = 0;
#else						 /* Default to I2S Format */
	/* 1-bit delay for I2S format */
	mcbsp->mcbsplp_xcr2_reg = MCBSPLP_XCR_XDATDLY_1;
	mcbsp->mcbsplp_rcr2_reg = MCBSPLP_RCR_RDATDLY_1;
#endif

	/* 16-bit words */
	mcbsp->mcbsplp_xcr1_reg = MCBSPLP_XCR_XWDLEN_16;
	mcbsp->mcbsplp_rcr1_reg = MCBSPLP_RCR_RWDLEN_16;

	if (omap35xx->play_strm.voices > 1)
	{
		/* TX: Dual Phase (Stereo), 16-bit word in Phase 2 */
		mcbsp->mcbsplp_xcr2_reg |= MCBSPLP_XCR_XPHASE | MCBSPLP_XCR_XWDLEN_16;
	}

	if (omap35xx->cap_strm.voices > 1)
	{
		/* RX: Dual Phase (Stereo), 16-bit words in Phase 2 */
		mcbsp->mcbsplp_rcr2_reg |= MCBSPLP_RCR_RPHASE | MCBSPLP_RCR_RWDLEN_16;
	}

	/* Calculate the CLK divider and the FS pulse width. 
	 * Program 1 less the desired value as per the docs. 
	 */
	frame_width = 16 * omap35xx->play_strm.voices;		   /* frame_width = sample_size * voices */
	divider = OMAP35XX_MASTER_CLK / (OMAP35XX_FRAME_RATE * frame_width);
	mcbsp->mcbsplp_srgr1_reg = (divider - 1) | (((frame_width / 2) - 1) << 8);

#endif

	/* Clear the GSYNC option, Derive SRG from CLKS pin. 
	 * Program 1 less the desired value (per docs) for the frame period 
	 */
	mcbsp->mcbsplp_srgr2_reg =
		MCBSPLP_SRGR2_REG_FSGM | ((frame_width - 1) & MCBSPLP_SRGR2_REG_FPER_MSK);

	/*  Program 1 less the desired value (as per docs) */
	mcbsp->mcbsplp_thrsh2_reg = 15;						   /* McBSP TxFifo threshold */
	mcbsp->mcbsplp_thrsh1_reg = 15;						   /* McBSP RxFifo threshold */

	/* Disable transmitter and receiver */
	mcbsp->mcbsplp_xccr_reg |= MCBSPLP_XCCR_REG_XDISABLE;
	mcbsp->mcbsplp_rccr_reg |= MCBSPLP_RCCR_REG_RDISABLE;

	if (omap35xx->clk_mode == MASTER)
	{
		/* Bring Sample-rate generator generator and transmitter out of reset */
		mcbsp->mcbsplp_spcr2_reg |= MCBSPLP_SPCR2_REG_GRST | MCBSPLP_SPCR2_REG_XRST;
		mcbsp->mcbsplp_spcr1_reg |= MCBSPLP_SPCR1_REG_RRST;
		/* Bring Frame-Sync generator out of reset. Must be done after
		 * transmitter is enabled to ensure transmitter is in sync.
		 */
		nanospin_ns (1000);
		mcbsp->mcbsplp_spcr2_reg |= MCBSPLP_SPCR2_REG_FRST;
	}
	else
	{
		/* Bring transmitter and receiver out of reset */
		mcbsp->mcbsplp_spcr2_reg |= MCBSPLP_SPCR2_REG_XRST;
		mcbsp->mcbsplp_spcr1_reg |= MCBSPLP_SPCR1_REG_RRST;
	}

	ado_mutex_unlock (&omap35xx->hw_lock);
	return EOK;
}

ado_dll_version_t ctrl_version;
void
ctrl_version (int *major, int *minor, char *date)
{
	*major = ADO_MAJOR_VERSION;
	*minor = 1;
	date = __DATE__;
}

int
omap35xx_parse_commandline (HW_CONTEXT_T * omap35xx, char *args)
{
	int     opt = 0;
	uint8_t mcbsp_idx;
	char   *value;
	char   *opts[] = {
		"i2c_addr",
		"mcbsp",
		"tx_voices",
		"rx_voices",
		"en_power_amp",
		"clk_mode",
		NULL
	};

	omap35xx->codec_i2c_addr = DEFAULT_I2C_ADDR;
	omap35xx->play_strm.voices = OMAP35XX_TX_VOICES;
	omap35xx->cap_strm.voices = OMAP35XX_RX_VOICES;
	omap35xx->clk_mode = OMAP35XX_CLK_MODE;
	mcbsp_idx = DEFAULT_MCBSP;

	while (args != NULL && args[0] != 0)
	{
		switch ((opt = getsubopt (&args, opts, &value)))
		{
		case 0:
			omap35xx->codec_i2c_addr = atoi (value);
			ado_debug (DB_LVL_DRIVER, "OMAP35xx: i2c_addr %d", omap35xx->codec_i2c_addr);
			break;
		case 1:
			mcbsp_idx = atoi (value);
			if (mcbsp_idx < 1 || mcbsp_idx > 5)
			{
				ado_error ("Invalid McBSP index");
				return EINVAL;
			}
			break;
		case 2:
			omap35xx->play_strm.voices = atoi (value);
			if (omap35xx->play_strm.voices < 1 || omap35xx->play_strm.voices > 4)
			{
				ado_error ("Invalid tx voices");
				return EINVAL;
			}
			break;
		case 3:
			omap35xx->cap_strm.voices = atoi (value);
			if (omap35xx->cap_strm.voices < 1 || omap35xx->cap_strm.voices > 2)
			{
				ado_error ("Invalid rx voices");
				return EINVAL;
			}
			break;
		case 4:
			omap35xx->en_power_amp = 1;
			break;
		case 5:
			omap35xx->clk_mode = atoi (value);
			if (omap35xx->clk_mode > 1 || omap35xx->clk_mode < 0)
			{
				ado_error ("Invalid clk mode");
				return EINVAL;
			}
			break;
		default:
			break;
		}
	}

	switch (mcbsp_idx)
	{
	case 1:
		omap35xx->mcbsp_addr = MCBSP1_BASE_ADDR;
		omap35xx->mcbsp_tx_dma_req = S_DMA_30;
		omap35xx->mcbsp_rx_dma_req = S_DMA_31;
		break;
	case 2:
		omap35xx->mcbsp_addr = MCBSP2_BASE_ADDR;
		omap35xx->mcbsp_tx_dma_req = S_DMA_32;
		omap35xx->mcbsp_rx_dma_req = S_DMA_33;
		break;
	case 3:
		omap35xx->mcbsp_addr = MCBSP3_BASE_ADDR;
		omap35xx->mcbsp_tx_dma_req = S_DMA_16;
		omap35xx->mcbsp_rx_dma_req = S_DMA_17;
		break;
	case 4:
		omap35xx->mcbsp_addr = MCBSP4_BASE_ADDR;
		omap35xx->mcbsp_tx_dma_req = S_DMA_18;
		omap35xx->mcbsp_rx_dma_req = S_DMA_19;
		break;
	case 5:
		omap35xx->mcbsp_addr = MCBSP5_BASE_ADDR;
		omap35xx->mcbsp_tx_dma_req = S_DMA_20;
		omap35xx->mcbsp_rx_dma_req = S_DMA_21;
		break;
	default:
		break;
	}

	ado_debug (DB_LVL_DRIVER, "OMAP35xx: Using McBSP%d, base addr = 0x%lx", mcbsp_idx,
		omap35xx->mcbsp_addr);

	return EOK;
}

ado_ctrl_dll_init_t ctrl_init;
int
ctrl_init (HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	omap35xx_t *omap35xx;

	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_INIT: omap35xx");

	if ((omap35xx = (omap35xx_t *) ado_calloc (1, sizeof (omap35xx_t))) == NULL)
	{
		ado_error ("Unable to allocate memory for omap35xx (%s)", strerror (errno));
		return -1;
	}
	*hw_context = omap35xx;

	if (omap35xx_parse_commandline (omap35xx, args) != EOK)
	{
		ado_free (omap35xx);
		return -1;
	}

	ado_card_set_shortname (card, "OMAP35xx");
	ado_card_set_longname (card, "OMAP35xx", omap35xx->mcbsp_addr);

	omap35xx->mcbsp =
		mmap_device_memory (0, sizeof (mcbsp_t), PROT_READ | PROT_WRITE | PROT_NOCACHE, 0,
		omap35xx->mcbsp_addr);

	if (omap35xx->mcbsp == MAP_FAILED)
	{
		ado_error ("Unable to mmap McBSP (%s) \n", strerror (errno));
		ado_free (omap35xx);
		return -1;
	}

	ado_mutex_init (&omap35xx->hw_lock);

	if (mcbsp_init (omap35xx) != EOK)
	{
		ado_error ("Failed to init McBSP\n");
		ado_mutex_destroy (&omap35xx->hw_lock);
		munmap_device_memory (omap35xx->mcbsp, sizeof (mcbsp_t));
		ado_free (omap35xx);
		return -1;
	}

	if (omap35xx_dma_attach (omap35xx, card))
	{
		ado_mutex_destroy (&omap35xx->hw_lock);
		munmap_device_memory (omap35xx->mcbsp, sizeof (mcbsp_t));
		ado_free (omap35xx);
		return -1;
	}

	omap35xx->play_strm.pcm_caps.chn_flags = SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
		SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER |
		SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;

	omap35xx->play_strm.pcm_caps.formats = SND_PCM_FMT_S16_LE;
	omap35xx->play_strm.pcm_caps.rates = OMAP35XX_PCM_RATES;
	omap35xx->play_strm.pcm_caps.min_rate = OMAP35XX_FRAME_RATE;
	omap35xx->play_strm.pcm_caps.max_rate = OMAP35XX_FRAME_RATE;
	omap35xx->play_strm.pcm_caps.min_voices = omap35xx->play_strm.voices;
	omap35xx->play_strm.pcm_caps.max_voices = omap35xx->play_strm.voices;

	omap35xx->play_strm.pcm_caps.min_fragsize = 128;
	omap35xx->play_strm.pcm_caps.max_fragsize = 32 * 1024;

	memcpy (&omap35xx->cap_strm.pcm_caps, &omap35xx->play_strm.pcm_caps,
		sizeof (omap35xx->cap_strm.pcm_caps));

	omap35xx->cap_strm.pcm_caps.min_voices = omap35xx->cap_strm.voices;
	omap35xx->cap_strm.pcm_caps.max_voices = omap35xx->cap_strm.voices;

	omap35xx->play_strm.pcm_funcs.capabilities = omap35xx_capabilities;
	omap35xx->play_strm.pcm_funcs.aquire = omap35xx_playback_aquire;
	omap35xx->play_strm.pcm_funcs.release = omap35xx_playback_release;
	omap35xx->play_strm.pcm_funcs.prepare = omap35xx_prepare;
	omap35xx->play_strm.pcm_funcs.trigger = omap35xx_playback_trigger;
	omap35xx->play_strm.pcm_funcs.position = omap35xx_position;

	omap35xx->cap_strm.pcm_funcs.capabilities = omap35xx_capabilities;
	omap35xx->cap_strm.pcm_funcs.aquire = omap35xx_capture_aquire;
	omap35xx->cap_strm.pcm_funcs.release = omap35xx_capture_release;
	omap35xx->cap_strm.pcm_funcs.prepare = omap35xx_prepare;
	omap35xx->cap_strm.pcm_funcs.trigger = omap35xx_capture_trigger;
	omap35xx->cap_strm.pcm_funcs.position = omap35xx_position;

	if (ado_pcm_create (card, "OMAP35xx PCM 0", 0, "omap35xx",
			MCBSP_PLAYBACK_PATH == ENABLED ? 1 : 0, &omap35xx->play_strm.pcm_caps,
			&omap35xx->play_strm.pcm_funcs, MCBSP_CAPTURE_PATH == ENABLED ? 1 : 0,
			&omap35xx->cap_strm.pcm_caps, &omap35xx->cap_strm.pcm_funcs, &omap35xx->pcm1))
	{
		ado_error ("Unable to create PCM devices (%s)", strerror (errno));
		omap35xx_dma_detach (omap35xx);
		ado_mutex_destroy (&omap35xx->hw_lock);
		munmap_device_memory (omap35xx->mcbsp, sizeof (mcbsp_t));
		ado_free (omap35xx);
		return -1;
	}

	if (codec_mixer (card, omap35xx))
	{
		ado_error ("Unable to initialize mixer (%s)", strerror (errno));
		omap35xx_dma_detach (omap35xx);
		ado_mutex_destroy (&omap35xx->hw_lock);
		munmap_device_memory (omap35xx->mcbsp, sizeof (mcbsp_t));
		ado_free (omap35xx);
		return -1;
	}

	return 0;
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int
ctrl_destroy (HW_CONTEXT_T * omap35xx)
{
	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_DESTROY: omap35xx");

	omap35xx_dma_detach (omap35xx);
	ado_mutex_destroy (&omap35xx->hw_lock);
	munmap_device_memory (omap35xx->mcbsp, sizeof (mcbsp_t));
	ado_free (omap35xx);
	return 0;
}

__SRCVERSION
	("$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/deva/ctrl/omap35xx/omap35xx_dll.c $ $Rev: 249398 $");

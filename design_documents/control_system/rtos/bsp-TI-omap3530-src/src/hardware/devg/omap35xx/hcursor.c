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

#include <string.h>
#include <unistd.h>

#include "omap.h"

/* private local constants */
#define OMAP_HCURSOR_RGB565_BPP 2

int omap_set_hw_cursor(disp_adapter_t *adapter, int dispno, uint8_t * bmp0,
		uint8_t * bmp1, unsigned color0, unsigned color1, int hotspot_x,
		int hotspot_y, int size_x, int size_y, int bmp_stride) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	uint16_t code;
	int byte = 0, bit;
	int x, y;
	uint32_t transparent_color;
	TRACE;

	/* use sw cursor  hcusror is set in configuration file omap35xx.conf*/
	if (o_ctx->hcursor != 1) {
		return -1;
	}

	/* cursor fits with in the display */
	if (size_x > o_ctx->width || size_y > o_ctx->height)
		return -1;

	/* set width and height */
	o_ctx->cursor_width = size_x;
	o_ctx->cursor_height = size_y;
		
	/* convert colors to rgb565 */
	color0 = disp_color_translate(NULL, DISP_SURFACE_FORMAT_RGB888,
			DISP_SURFACE_FORMAT_RGB565, color0);
	color1 = disp_color_translate(NULL, DISP_SURFACE_FORMAT_RGB888,
			DISP_SURFACE_FORMAT_RGB565, color1);
			
			
	/* create unique color for transpart color,  
	 * increment color0 and check it is not color1 
	 */
	transparent_color = color0 + 2; 
	if (transparent_color == color1)
		transparent_color = color0 + 4;

	/* allocate memory for hardware cursor image */
	if (o_ctx->cursor_vptr == NULL || o_ctx->cursor_size != (size_x * size_y
			* OMAP_HCURSOR_RGB565_BPP)) {
		if (o_ctx->cursor_vptr)
			munmap(o_ctx->cursor_vptr, o_ctx->cursor_size);
		o_ctx->cursor_size = size_x * size_y * OMAP_HCURSOR_RGB565_BPP;
		o_ctx->cursor_vptr = mmap(0, o_ctx->cursor_size, PROT_READ|PROT_WRITE|(o_ctx->cached_mem ? 0 : PROT_NOCACHE)
				, MAP_ANON|MAP_PHYS|MAP_PRIVATE, NOFD, 0);
		if (o_ctx->cursor_vptr == MAP_FAILED)
			return -1;
		o_ctx->cursor_paddr = disp_phys_addr(o_ctx->cursor_vptr);
	}

	/* combine bitmaps and transparent color to create rgb565 image */
	for (y = 0; y < size_y; y++) {
		/* next line in bitmap */
		byte = y * bmp_stride;
		bit = 0x80; /* MSB */
		for (x = 0; x < size_x; x++) {
			code = transparent_color;
			if (bmp0[byte] & bit)
				code = color0;
			if (bmp1[byte] & bit)
				code = color1;
			o_ctx->cursor_vptr[((y*size_x)+x)] = code;
			/* index into source bitmops */
			bit >>= 1;
			if (bit == 0) {
				/* next byte */
				bit = 0x80;
				byte++;
			}
		}
	}

	/* OMAP_DISPC_CONFIG bits
	 *
	 * TCKLCDSELECTION (?<<11) Transparency color key selection (LCD output) RW 0
	 *	0x0: Graphics destination transparency color key selected
	 *	0x1: Video source transparency color key selected
	 * TCKLCDENABLE (?<<10) Transparency color key enabled (LCD output) RW 0
	 *  	0x0: Disable the transparency color key for the LCD
	 *	0x1: Enable the transparency color key for the LCD
	 */
	*OMAP_DISPC_CONFIG |= (1<<10);

	/* set transparent color */
	*OMAP_DISPC_TRANS_COLOR = transparent_color;
	/* set GFX buffer to the cursor image */
	*OMAP_DISPC_GFX_BA0 = o_ctx->cursor_paddr;
	/*
	 *      0x6<<1 for rgb565
	 */
	*OMAP_DISPC_GFX_ATTRIBUTES |= (0x06 << 1);
	/* Cursor image size */
	*OMAP_DISPC_GFX_SIZE = (size_y-1) << 16 | (size_x-1);
	*OMAP_DISPC_GFX_ROW_INC = 1;
	*OMAP_DISPC_GFX_PIXEL_INC = 1;

	return 0;
}

void omap_enable_hw_cursor(disp_adapter_t *adapter, int dispno) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	TRACE;
	*OMAP_DISPC_GFX_ATTRIBUTES |= 1;
	/* Wait for any outstanding update */
	while ((*OMAP_DISPC_CONTROL & (1<<5)) != 0) {
		usleep(100);
	}
	/* Hit GOBIT and enabled  LCD  */
	*OMAP_DISPC_CONTROL |= (1<<5) | 1;
}

void omap_disable_hw_cursor(disp_adapter_t *adapter, int dispno) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	TRACE;
	*OMAP_DISPC_GFX_ATTRIBUTES &= ~1;
	/* Wait for any outstanding update */
	while ((*OMAP_DISPC_CONTROL & (1<<5)) != 0) {
		usleep(100);
	}
	/* Hit GOBIT and enabled  LCD  */
	*OMAP_DISPC_CONTROL |= (1<<5) | 1;
}

void omap_set_hw_cursor_pos(disp_adapter_t *adapter, int dispno, int x, int y) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	TRACE;

	/* keep cursor within screen */
	if (x < 0) {
		x = 0;
	}
	if (y < 0) {
		y = 0;
	}

	if (y > ((o_ctx->height-o_ctx->cursor_height)-1))
		y = (o_ctx->height-o_ctx->cursor_height)-1;
	if (x > ((o_ctx->width-o_ctx->cursor_width)-1))
		x = (o_ctx->width-o_ctx->cursor_width)-1;

	*OMAP_DISPC_GFX_POSITION = (y << 16) | x;

	/* Wait for any outstanding update */
	while ((*OMAP_DISPC_CONTROL & (1<<5)) != 0) {
		usleep(100);
	}
	/* Hit GOBIT and enabled  LCD  */
	*OMAP_DISPC_CONTROL |= (1<<5) | 1;

}

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devg/omap35xx/hcursor.c $ $Rev: 249398 $" );

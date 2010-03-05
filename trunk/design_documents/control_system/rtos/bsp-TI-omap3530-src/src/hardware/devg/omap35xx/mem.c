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

#include <stdlib.h>

#include "omap.h"
#define WIDTH_GRANULARITY 16
#define ALIGNMENT 4

int omap_mem_init(disp_adapter_t *adapter, char *optstring) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	o_ctx->cachectl.fd = NOFD;
	if(cache_init(0, &o_ctx->cachectl, NULL) == -1) {
		slogf(21,0,"cache_init failed");
	}


	return 0;
}

void omap_mem_fini(disp_adapter_t *adapter) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	cache_fini(&o_ctx->cachectl);
}

int omap_mem_reset(disp_adapter_t *adapter, disp_surface_t *surf) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return 0;
}

disp_surface_t * omap_alloc_surface(disp_adapter_t *adapter, int width,
		int height, unsigned format, unsigned flags, unsigned user_flags) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return NULL;
}

int omap_free_surface(disp_adapter_t *adapter, disp_surface_t *surf) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return -1;
}

unsigned long omap_mem_avail(disp_adapter_t *adapter, unsigned flags) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return 0;
}

int omap_query_apertures(disp_adapter_t *adapter, disp_aperture_t *ap) {
	omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return 0;
}

int omap_query_surface(disp_adapter_t *adapter, disp_surface_t *surf,
		disp_surface_info_t *info) {
			omap_ctx_t	*o_ctx = adapter->ms_ctx;
	TRACE;
	return 0;
}

/*
 * If a client of the driver wants to allocate memory itself,
 * it must allocate it in accordance with the parameters returned by
 * this function.  Since this memory will not be coming from
 * video memory, we must check the flags accordingly.
 */
int omap_get_alloc_info(disp_adapter_t *adapter, int width, int height,
		unsigned format, unsigned flags, unsigned user_flags,
		disp_alloc_info_t *info) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	int			bpp = DISP_BYTES_PER_PIXEL(format);
	int stride;
	TRACE;
	
	stride = width * bpp;

	info->start_align = ALIGNMENT;
	info->end_align = ALIGNMENT;
	info->min_stride = stride;
	info->max_stride = ~0;
	info->stride_gran = WIDTH_GRANULARITY;
	info->map_flags = DISP_MAP_PHYS;
	info->prot_flags = DISP_PROT_READ | DISP_PROT_WRITE | (o_ctx->cached_mem ? 0 : DISP_PROT_NOCACHE) ;
	info->surface_flags = (DISP_SURFACE_CPU_LINEAR_READABLE
			| DISP_SURFACE_CPU_LINEAR_WRITEABLE | DISP_SURFACE_PHYS_CONTIG
			| DISP_SURFACE_DISPLAYABLE );

	return 0;
}

int omap_get_alloc_layer_info(disp_adapter_t *adapter, int dispno[],
		int layer_idx[], int nlayers, unsigned format, int surface_index,
		int width, int height, unsigned sflags, unsigned hint_flags,
		disp_alloc_info_t *info) {
	omap_ctx_t *o_ctx = adapter->ms_ctx;
	unsigned alloc_format;
	int stride, i;
	int bpp;

	if (surface_index > 0) {
		DEBUG_MSG("surface_index > 0");
		return -1;
	}

	switch (format) {
	case DISP_LAYER_FORMAT_RGB565:
		alloc_format = DISP_SURFACE_FORMAT_RGB565;
		bpp = 2;
		break;
	case DISP_LAYER_FORMAT_RGB888:
		alloc_format = DISP_SURFACE_FORMAT_RGB888;
		bpp=3;
		break;
	case DISP_LAYER_FORMAT_ARGB8888:
		alloc_format = DISP_SURFACE_FORMAT_ARGB8888;
		bpp=4;
		break;
	case DISP_LAYER_FORMAT_YUY2:
		alloc_format = DISP_SURFACE_FORMAT_PACKEDYUV_YUY2;
		bpp=2;
		break;
	case DISP_LAYER_FORMAT_UYVY:
		alloc_format = DISP_SURFACE_FORMAT_PACKEDYUV_UYVY;
		bpp=2;
		break;
	case DISP_LAYER_FORMAT_V422:
		alloc_format = DISP_SURFACE_FORMAT_PACKEDYUV_V422;
		bpp=2;
		break;
	case DISP_LAYER_FORMAT_YVYU:
		alloc_format = DISP_SURFACE_FORMAT_PACKEDYUV_YVYU;
		bpp=2;
		break;
	default:
		SLOG_ERROR("format unknown");
		return -1;
	}


	for (i = 0; i < nlayers; i++) {
		switch (layer_idx[i]) {
		case OMAP_LAYER_GFX:
			if (format == DISP_LAYER_FORMAT_YUY2 || format
					== DISP_LAYER_FORMAT_UYVY) {
				SLOG_ERROR("Format not supported for layer");
				return -1;
			}
			break;
		case OMAP_LAYER_VID1:
			if (format == DISP_LAYER_FORMAT_ARGB8888) {
				SLOG_ERROR("Format not supported for layer");
				return -1;
			}
			break;
		case OMAP_LAYER_VID2:
			break;
		default:
			SLOG_ERROR("format unknown");
			return -1;
		}
	}
	stride = width * bpp;

	info->start_align = ALIGNMENT;
	info->end_align = ALIGNMENT;
	info->min_stride = stride;
	info->max_stride = ~0;
	info->stride_gran = WIDTH_GRANULARITY;
	info->map_flags = DISP_MAP_PHYS;
	info->prot_flags = DISP_PROT_READ | DISP_PROT_WRITE | (o_ctx->cached_mem ? 0 : DISP_PROT_NOCACHE) ;
	info->surface_flags = (DISP_SURFACE_CPU_LINEAR_READABLE
			| DISP_SURFACE_CPU_LINEAR_WRITEABLE | DISP_SURFACE_PHYS_CONTIG
			| DISP_SURFACE_DISPLAYABLE );

	return 0;
}

int devg_get_memfuncs(disp_adapter_t *adapter, disp_memfuncs_t *funcs,
		int tabsize) {
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, init, omap_mem_init, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, fini, omap_mem_fini, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, module_info, omap_module_info,
			tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, reset, omap_mem_reset, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, alloc_surface, omap_alloc_surface,
			tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, free_surface, omap_free_surface,
			tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, mem_avail, omap_mem_avail, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_apertures,
			omap_query_apertures, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_surface, omap_query_surface,
			tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_info, omap_get_alloc_info,
			tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_layer_info,
			omap_get_alloc_layer_info, tabsize);

	return 0;
}


__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devg/omap35xx/mem.c $ $Rev: 249398 $" );

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef _DPU_SATURN_H_
#define _DPU_SATURN_H_

#include "../spacemit_crtc.h"

struct dpu_format_id {
	u32 format;		/* DRM fourcc */
	u8 id;
	u8 bpp;			/* bits per pixel */
};

#define SPACEMIT_DPU_INVALID_FORMAT_ID	0xff

enum format_features {
	FORMAT_RGB  = BIT(0),
};

struct spacemit_hw_rdma {
	u16 formats;
};

int dpu_parse_clocks(struct device *dev, struct dpu_clk_context *clk_ctx);
u8 spacemit_plane_hw_get_format_id(u32 format);
u32 saturn_conf_dpuctrl_rdma(struct spacemit_crtc *a_crtc);
extern const struct spacemit_hw_device k3_saturn_dpu;
void saturn_enable_irq_mask(struct spacemit_crtc *a_crtc, bool enable,
			    u32 offset, u32 mask);
void spacemit_cfg_rdy_timer_handler(struct timer_list *t);
#endif

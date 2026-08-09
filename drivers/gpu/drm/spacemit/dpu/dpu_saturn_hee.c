// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/pm_qos.h>
#include <linux/regmap.h>
#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include "dpu_saturn.h"
#include "saturn_fbcmem.h"
#include "../spacemit_cmdlist.h"
#include "../spacemit_dmmu.h"
#include "../spacemit_dpu_reg.h"
#include <video/display_timing.h>
#include "../spacemit_dpu_ids.h"
#include "saturn_regs/ops_hee.h"
#include "saturn_regs/reg_map_hee.h"

#include "dpu_trace.h"

#define SATURN_WRITE_TBU_REGS(hwdev, base, reg_id, cl_tbu) \
do {\
	dpu_write(hwdev, MMU_TBU_REG, base, tbu_base_addr##reg_id##_low, \
		  tbu->ttb_pa[reg_id] & 0xFFFFFFFF, cl_tbu); \
	dpu_write(hwdev, MMU_TBU_REG, base, tbu_base_addr##reg_id##_high, \
		  (tbu->ttb_pa[reg_id] >> 32) & 0xFFF, cl_tbu); \
	dpu_write(hwdev, MMU_TBU_REG, base, tbu_va##reg_id, \
		 (tbu->tbu_va[reg_id] >> 12), cl_tbu); \
	dpu_write(hwdev, MMU_TBU_REG, base, \
		  tbu_size##reg_id, tbu->ttb_size[reg_id], cl_tbu); \
	dpu_write(hwdev, MMU_TBU_REG, base, \
		  vsync_update_en, 1, cl_tbu); \
} while (0)

#define SATURN_WRITE_RDMA_ADDR_REG(hwdev, reg_id, rdma_id, addr, cl_rdma) \
do { \
	if (hwdev) { \
		dpu_write(hwdev, RDMA_PATH_X_REG, RDMA_BASE_ADDR[rdma_id], \
			  base_addr##reg_id##_low_ly0, (addr) & 0xFFFFFFFF, \
			  cl_rdma); \
		dpu_write(hwdev, RDMA_PATH_X_REG, RDMA_BASE_ADDR[rdma_id], \
			  base_addr##reg_id##_high_ly0, ((addr) >> 32) & 0xFFF, \
			  cl_rdma); \
	} \
} while (0)

static const u32 RDMA_BASE_ADDR[] = {
	RDMA0_BASE_ADDR,
	RDMA1_BASE_ADDR,
	RDMA2_BASE_ADDR,
	RDMA3_BASE_ADDR,
};

static const u32 CMP_BASE_ADDR[] = {
	CMP0_BASE_ADDR,
	CMP1_BASE_ADDR,
	CMP2_BASE_ADDR,
};

static const u32 TMG_BASE_ADDR[] = {
	0,
	TMG0_BASE_ADDR,
	0,
};

static const u32 MMU_TBU_BASE_ADDR_ARRAY[] = {
	TBU0_ADDR, TBU1_ADDR, TBU2_ADDR, TBU3_ADDR,
	TBU4_ADDR, TBU5_ADDR, TBU6_ADDR, TBU7_ADDR,
};

void saturn_hee_enable_vsync(struct spacemit_crtc *a_crtc, struct spacemit_hw_device *hwdev,
			     bool enable)
{
	u32 offset = DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_MSK;
	u32 mask = saturn_hee_get_irq_bit(INT_VSYNC, a_crtc->dev_id);

	saturn_enable_irq_mask(a_crtc, enable, offset, mask);
}

void saturn_hee_enable_cfg_irq(struct spacemit_crtc *a_crtc, struct spacemit_hw_device *hwdev,
			       bool enable)
{
	u32 offset = DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_MSK;
	u32 mask = saturn_hee_get_irq_bit(INT_CFG_RDY, a_crtc->dev_id);

	saturn_enable_irq_mask(a_crtc, enable, offset, mask);
}

void saturn_hee_cfg_ready(struct spacemit_crtc *a_crtc,
			  struct spacemit_hw_device *hwdev)
{
	u32 base = DPU_SCENE_CTL_ADDR(a_crtc->dev_id);

	saturn_enable_irq_mask(a_crtc, true,
		DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_MSK,
		saturn_hee_get_irq_bit(INT_UNDERRUN, a_crtc->dev_id));
	a_crtc->ur_mask_armed = true;
	dpu_write(hwdev, DPU_CTL_REG, base, both_cfg_rdy, 1);
}

void saturn_hee_sw_start(struct spacemit_crtc *a_crtc,
			 struct spacemit_hw_device *hwdev)
{
	u32 base;

	base = DPU_SCENE_CTL_ADDR(a_crtc->dev_id);
	dpu_write_reg(hwdev, DPU_CTL_REG, base, sw_start, 1);
}

static void saturn_init_tmg(struct spacemit_crtc *a_crtc)
{
	u32 base = 0;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_display_mode *mode = &crtc->mode;
	u16 vfp, vbp, vsync, hfp, hbp, hsync;
	void __iomem *tmg_addr;
	u32 value;

	/* dev_id names the composer type, not the DPU instance: use TMG0. */
	tmg_addr = hwdev->base + TMG0_BASE_ADDR;

	hsync = mode->hsync_end - mode->hsync_start;
	hbp = mode->htotal - mode->hsync_end;
	hfp = mode->hsync_start - mode->hdisplay;
	vsync = mode->vsync_end - mode->vsync_start;
	vbp = mode->vtotal - mode->vsync_end;
	vfp = mode->vsync_start - mode->vdisplay;
	trace_drm_display_mode_info(mode);

	base = TMG_BASE_ADDR[a_crtc->dev_id];
	if (base) {
		dpu_write(hwdev, TMG_REG, base, disp_ready_man_en, 0);
		dpu_write(hwdev, TMG_REG, base, background_r, 0);
		dpu_write(hwdev, TMG_REG, base, background_g, 0x0);
		dpu_write(hwdev, TMG_REG, base, background_b, 0xff);
		dpu_write(hwdev, TMG_REG, base, eof_1st_ln_dly_num, vfp - 7);
		dpu_write(hwdev, TMG_REG, base, eof_2nd_ln_dly_num, vfp - 6);
		dpu_write(hwdev, TMG_REG, base, split_en, 0);
		dpu_write(hwdev, TMG_REG, base, cmd_screen, 0);
		dpu_write(hwdev, TMG_REG, base, cmd_wait_en, 0);
		dpu_write(hwdev, TMG_REG, base, cmd_wait_te, 0);
		dpu_write(hwdev, TMG_REG, base, sof_pre_ln_num, 0);
		dpu_write(hwdev, TMG_REG, base, hfp, hfp);
		dpu_write(hwdev, TMG_REG, base, hbp, hbp);
		dpu_write(hwdev, TMG_REG, base, vfp, vfp);
		dpu_write(hwdev, TMG_REG, base, vbp, vbp);
		dpu_write(hwdev, TMG_REG, base, hsync_width, hsync);
		dpu_write(hwdev, TMG_REG, base, vsync_width, vsync);
		dpu_write(hwdev, TMG_REG, base, hsp, 1);
		dpu_write(hwdev, TMG_REG, base, vsp, 1);
		dpu_write(hwdev, TMG_REG, base, h_active, mode->hdisplay);
		dpu_write(hwdev, TMG_REG, base, v_active, mode->vdisplay);
		dpu_write(hwdev, TMG_REG, base, fm_timing_en, 1);
		dpu_write(hwdev, TMG_REG, base, user, a_crtc->out_format);
	}

	value = readl(tmg_addr + 0x14);
	value &= 0xFFFF0000;
	value |= vfp;
	writel(value, tmg_addr + 0x14);

	/* align vsync with hsync */
	value = readl(tmg_addr + 0x3c);
	value |= 0x01;
	writel(value, tmg_addr + 0x3c);
}

static void saturn_init_regs(struct spacemit_crtc *a_crtc)
{
	u32 base = 0;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	base = CMP_BASE_ADDR[a_crtc->dev_id];
	dpu_write(hwdev, CMPS_X_REG, base, bg_color_b, 0x0);
	dpu_write(hwdev, CMPS_X_REG, base, bg_color_r, 0x0);
	dpu_write(hwdev, CMPS_X_REG, base, bg_color_g, 0x0);
	dpu_write(hwdev, CMPS_X_REG, base, bg_color_a, 0xFF);
	dpu_write(hwdev, CMPS_X_REG, base, module_enable, 1);
	dpu_write(hwdev, CMPS_X_REG, base, layer00_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer01_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer02_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer03_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer04_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer05_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer06_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer07_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer08_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer09_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer10_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer11_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer12_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer13_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer14_en, 0);
	dpu_write(hwdev, CMPS_X_REG, base, layer15_en, 0);

	base = DPU_TOP_BASE_ADDR;

	base = EE_ADDR;
	dpu_write(hwdev, EE_REG, base, m_benable, 0);

	saturn_init_tmg(a_crtc);
}

static void saturn_setup_dma_top(struct spacemit_crtc *a_crtc)
{
	u32 base = DMA_TOP_BASE_ADDR;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	dpu_write(hwdev, DMA_TOP_REG, base, image_rr_ratio, 0x10);
	dpu_write(hwdev, DMA_TOP_REG, base, pixel_num_th, 4);

	dpu_write(hwdev, DMA_TOP_REG, base, rdma_timeout_limit, 0xFFFE);
	dpu_write(hwdev, DMA_TOP_REG, base, wdma_timeout_limit, 0xFFFF);
}

static void saturn_setup_mmu_top(struct spacemit_crtc *a_crtc)
{
	unsigned int rd_outs_num = 0;
	u32 base = MMU_TOP_BASE_ADDR;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	rd_outs_num = RD_OUTS_NUM / 2;

	dpu_write(hwdev, MMU_TOP_REG, base, rdma_timelimit, RDMA_TIMELIMIT);

	dpu_write(hwdev, MMU_TOP_REG, base, sram0_tlb_axi_port_sel, 0);
	dpu_write(hwdev, MMU_TOP_REG, base, sram1_tlb_axi_port_sel, 0);
	dpu_write(hwdev, MMU_TOP_REG, base, dmac0_rd_outs_num, rd_outs_num);
	dpu_write(hwdev, MMU_TOP_REG, base, dmac1_rd_outs_num, rd_outs_num);
}

void saturn_hee_irq_enable(struct spacemit_crtc *a_crtc, bool enable)
{
	u32 offset = DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_MSK;
	u32 mask;

	trace_saturn_irq_enable("irq online", enable);
	mask = saturn_hee_get_irq_bit(INT_UNDERRUN, a_crtc->dev_id) |
	       saturn_hee_get_irq_bit(INT_CFG_RDY, a_crtc->dev_id);
	if (enable) {
		struct spacemit_drm_private *priv =
				a_crtc->crtc.dev->dev_private;

		/*
		 * Ack first: a surviving status bit would fire as the mask
		 * opens.
		 */
		regmap_write(priv->hwdev->regmap,
			     DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_STS, mask);
	}
	saturn_enable_irq_mask(a_crtc, enable, offset, mask);
	a_crtc->ur_mask_armed = true;
}

void saturn_hee_dpu_init(struct spacemit_crtc *a_crtc)
{
	saturn_init_regs(a_crtc);
	saturn_setup_dma_top(a_crtc);
	saturn_setup_mmu_top(a_crtc);
	saturn_hee_irq_enable(a_crtc, true);
}

#define SATURN_WRITE_COMPOSER_LAYER(id_name) \
do {\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _colorkey_en,	\
		  0, cl_cmp);					\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _blend_mode,	\
		  blend_mode, cl_cmp);				\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _blend_sel,	\
		  alpha_sel, cl_cmp);				\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _layer_alpha,	\
		  alpha, cl_cmp);					\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _alpha_ratio,	\
		  0, cl_cmp);					\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _area_left,	\
		  crtc_x, cl_cmp);					\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _area_top,	\
		  crtc_y, cl_cmp);					\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _area_right,	\
		  crtc_x + crtc_w - 1, cl_cmp);			\
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _area_bottom,	\
		  crtc_y + crtc_h - 1, cl_cmp);			\
	if (unlikely(solid_en)) { \
		dpu_write(hwdev, CMPS_X_REG, base,				\
			  layer ## id_name ## _solid_color_r,			\
			  solid_r, cl_cmp);			\
		dpu_write(hwdev, CMPS_X_REG, base,				\
			  layer ## id_name ## _solid_color_g,			\
			  solid_g, cl_cmp);			\
		dpu_write(hwdev, CMPS_X_REG, base,				\
			  layer ## id_name ## _solid_color_b,			\
			  solid_b, cl_cmp);			\
		dpu_write(hwdev, CMPS_X_REG, base,				\
			  layer ## id_name ## _solid_color_a,			\
			  solid_a, cl_cmp);			\
		dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _solid_en,	\
			  1, cl_cmp);				\
	} else { \
		dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _solid_en,	\
			  0, cl_cmp);				\
		dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _dma_id,	\
			  rdma_id, cl_cmp);			\
	} \
	dpu_write(hwdev, CMPS_X_REG, base, layer ## id_name ## _en, 1, cl_cmp); \
} while (0)

static void saturn_write_fbcmem_regs(struct spacemit_hw_device *hwdev,
				     struct drm_plane_state *state, u32 rdma_id,
				     u32 base, struct cmdlist_regs *cl_rdma)
{
	struct drm_crtc_state *crtc_state = state->crtc->state;
	const struct spacemit_crtc_rdma *rdmas =
			to_spacemit_crtc_state(crtc_state)->rdmas;
	u32 size  = rdmas[rdma_id].fbcmem.size;
	u32 start = rdmas[rdma_id].fbcmem.start;
	bool map   = rdmas[rdma_id].fbcmem.map;

	dpu_write_word(hwdev, base, 31, map << 28 | start << 16 | size,
		       cl_rdma);
}


void saturn_hee_plane_update_hw_channel(struct drm_plane *plane)
{
	struct drm_plane_state *state = plane->state;
	struct spacemit_crtc *a_crtc = to_spacemit_crtc(state->crtc);
	struct drm_framebuffer *fb = plane->state->fb;
	struct spacemit_plane_state *spacemit_plane_state =
			to_spacemit_plane_state(state);
	struct cmdlist_regs *cl_cmp = NULL;
	u32 rdma_id = spacemit_plane_state->rdma_id;
	u8 alpha = state->alpha >> 8;
	u16 pixel_alpha = state->pixel_blend_mode;
	u32 src_w, src_h, src_x, src_y;
	u32 crtc_w, crtc_h, crtc_x, crtc_y;
	u32 alpha_sel, blend_mode = 0;
	struct drm_display_mode *mode = &state->crtc->mode;

	u32 base;
	bool solid_en = false;
	u32 solid_a, solid_r, solid_g, solid_b;
	struct spacemit_drm_private *priv = plane->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct cmdlist_regs *cl_rdma = a_crtc->cl_rdma;

	trace_spacemit_plane_update_hw_channel("rdma_id", rdma_id);

	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;
	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;

	crtc_w = state->crtc_w;
	crtc_h = state->crtc_h;
	crtc_x = state->crtc_x;
	crtc_y = state->crtc_y;

	drm_dbg(plane->dev, "crtc_x %u crtc_y %u\n", crtc_x, crtc_y);

	if (rdma_id == RDMA_INVALID_ID)
		solid_en = true;

	trace_dpu_plane_info(state, fb, rdma_id, alpha, state->rotation);

	/* For solid color both src_w and src_h are 0 */
	if (!solid_en) {
		base = RDMA_BASE_ADDR[rdma_id];
		/* linear scanout only: the FBC decode path is not exposed */
		dpu_write(hwdev, RDMA_PATH_X_REG, base, layer_mode, 0, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, layer_cmpsr_id,
			  a_crtc->dev_id, cl_rdma);
		/*
		 * force online-mode routing whatever the bootloader left behind
		 */
		dpu_write(hwdev, RDMA_PATH_X_REG, base, is_offline, 0, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, is_two_layers, 0,
			  cl_rdma);

		dpu_write(hwdev, RDMA_PATH_X_REG, base, img_width_ly0,
			  fb->width, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, img_height_ly0,
			  fb->height, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, bbox_start_x_ly0, src_x,
			  cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, bbox_start_y_ly0, src_y,
			  cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, bbox_end_x_ly0,
			  src_x + src_w - 1, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, bbox_end_y_ly0,
			  src_y + src_h - 1, cl_rdma);

		saturn_write_fbcmem_regs(hwdev, state, rdma_id, base, cl_rdma);

		dpu_write(hwdev, RDMA_PATH_X_REG, base, rot_mode_ly0, 0,
			  cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, uv_swap, 0, cl_rdma);
		dpu_write(hwdev, RDMA_PATH_X_REG, base, pixel_format,
			  spacemit_plane_state->format, cl_rdma);
	} else {
		/* Constant-fill layer: zero-size source, transparent black. */
		solid_r = 0;
		solid_g = 0;
		solid_b = 0;
		solid_a = 0;
	}

	switch (pixel_alpha) {
	case DRM_MODE_BLEND_COVERAGE:
		blend_mode = 0x0;
		break;
	case DRM_MODE_BLEND_PREMULTI:
		blend_mode = 0x1;
		break;
	case DRM_MODE_BLEND_PIXEL_NONE:
		blend_mode = 0x0;
		break;
	default:
		drm_err(plane->dev, "unsupported blend mode for pixel alpha\n");
	}

	if (state->fb->format && state->fb->format->has_alpha &&
	    pixel_alpha != DRM_MODE_BLEND_PIXEL_NONE) {
		if (alpha != 0xff)
			alpha_sel = 0x2;
		else
			alpha_sel = 0x1;
	} else {
		blend_mode = 0x0;
		alpha_sel = 0x0;
	}

	cl_cmp = alloc_cmdlist_regs(CMPS_X_REG);

	base = CMP_BASE_ADDR[a_crtc->dev_id];
	dpu_write(hwdev, CMPS_X_REG, base, dst_w, mode->hdisplay, cl_cmp);
	dpu_write(hwdev, CMPS_X_REG, base, dst_h, mode->vdisplay, cl_cmp);

	SATURN_WRITE_COMPOSER_LAYER(00);
	cmdlist_regs_packing(crtc_to_cl(plane->state->crtc), CMDLIST_MOD_COMP,
			     cl_cmp);
	free_cmdlist_regs(cl_cmp);
}

void saturn_hee_plane_disable_hw_channel(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	u8 dev_id = to_spacemit_crtc(old_state->crtc)->dev_id;
	u32 base = CMP_BASE_ADDR[dev_id];
	u32 rdma_id = to_spacemit_plane_state(old_state)->rdma_id;
	struct spacemit_drm_private *priv = plane->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct cmdlist_regs *cl_cmp = NULL;

	trace_spacemit_plane_disable_hw_channel(0, rdma_id);
	cl_cmp = alloc_cmdlist_regs(CMPS_X_REG);
	dpu_write(hwdev, CMPS_X_REG, base, layer00_en, 0, cl_cmp);

	cmdlist_regs_packing(crtc_to_cl(old_state->crtc), CMDLIST_MOD_COMP,
			     cl_cmp);
	free_cmdlist_regs(cl_cmp);
}

void saturn_hee_conf_dpuctrl(struct drm_crtc *crtc,
			     struct drm_crtc_state *old_state)
{
	u32 rdma_en = 0;
	u32 base;
	u32 pp_base = POST_PIPE_ADDR;
	struct spacemit_drm_private *priv = crtc->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct spacemit_crtc *a_crtc = to_spacemit_crtc(crtc);
	struct drm_display_mode *mode = &crtc->mode;
	struct drm_crtc_state *crtc_state = a_crtc->crtc.state;
	struct spacemit_crtc_state *spacemit_crtc_state =
			to_spacemit_crtc_state(crtc_state);
	struct cmdlist *cl = NULL;
	struct cmdlist_regs *pp_cl = NULL;

	base = DPU_SCENE_CTL_ADDR(a_crtc->dev_id);
	rdma_en = saturn_conf_dpuctrl_rdma(a_crtc);

	pp_cl = alloc_cmdlist_regs(POSTPIPE_REG);
	/*
	 * postpipe should be configured no matter whether pq function is on or
	 * off
	 */
	dpu_write_word(hwdev, pp_base, 0, 0, pp_cl);
	dpu_write(hwdev, POSTPIPE_REG, pp_base, m_inwidth, mode->hdisplay,
		  pp_cl);
	dpu_write(hwdev, POSTPIPE_REG, pp_base, m_inheight, mode->vdisplay,
		  pp_cl);

	cmdlist_regs_packing(crtc_to_cl(crtc), CMDLIST_MOD_COMP, pp_cl);
	free_cmdlist_regs(pp_cl);

	cmdlist_sort_by_group(crtc);
	cmdlist_atomic_commit(crtc, old_state);

	dpu_write(hwdev, DPU_CTL_REG, base, nml_scl_en, 0);
	dpu_write(hwdev, DPU_CTL_REG, base, nml_rch_en, rdma_en);
	dpu_write(hwdev, DPU_CTL_REG, base, timing_inter0, 8);
	dpu_write(hwdev, DPU_CTL_REG, base, timing_inter1, 8);
	dpu_write(hwdev, DPU_CTL_REG, base, video_mod, 1);
	dpu_write(hwdev, DPU_CTL_REG, base, dbg_mod, 0);

	dpu_write(hwdev, DPU_CTL_REG, base, nml_outctl_en, 0x1);
	dpu_write(hwdev, DPU_CTL_REG, base, nml_frm_timing_en, 2);
	cl = &spacemit_crtc_state->cl;
	if (cl->va) {
		crtc_cmdlist_sort_by_group(a_crtc);
		crtc_cmdlist_atomic_commit(a_crtc);
	}
}

u32 saturn_hee_get_cfg_rdy(struct spacemit_crtc *a_crtc,
			   struct spacemit_hw_device *hwdev)
{
	u32 base = DPU_SCENE_CTL_ADDR(a_crtc->dev_id);

	return dpu_read_word(hwdev, base, 2);
}

u32 saturn_hee_get_int_sts(struct spacemit_hw_device *hwdev, int dev_id)
{
	/* Both K3 DPU instances are wired as COMPOSER1 (see master_bind). */
	if (dev_id != COMPOSER1)
		return 0;

	return dpu_read_word(hwdev, DPU_INT_BASE_ADDR, 12);
}

static const struct spacemit_crtc_irq_bitfield {
	enum spacemit_dpu_irq irq_id;
	u32 irq_bit;
} irq_list[] = {
	[INT_UNDERRUN] = {INT_UNDERRUN, DPU_INT_FRM_TIMING_UNFLOW},
	[INT_CFG_RDY] = {INT_CFG_RDY, DPU_INT_CFG_RDY_CLR},
	[INT_VSYNC] = {INT_VSYNC, DPU_INT_FRM_TIMING_VSYNC},
	[INT_EOF] = {INT_EOF, DPU_INT_FRM_TIMING_EOF},
	[INT_REST] = {INT_REST, DPU_REST_INT_BITS},
};

u32 saturn_hee_get_irq_bit(enum spacemit_dpu_irq irq_id, int dev_id)
{
	if (irq_id >= ARRAY_SIZE(irq_list))
		return 0;

	if (dev_id == COMPOSER1 || dev_id == COMPOSER2)
		return irq_list[irq_id].irq_bit;
	else
		return 0;
}

void saturn_hee_clr_int_sts(struct spacemit_crtc *a_crtc, u32 data, int dev_id)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	/* Both K3 DPU instances are wired as COMPOSER1 (see master_bind). */
	if (dev_id != COMPOSER1)
		return;

	regmap_write(hwdev->regmap, DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_STS,
		     data);
	if ((saturn_hee_get_irq_bit(INT_UNDERRUN, dev_id) & data) &&
	    a_crtc->ur_mask_armed) {
		saturn_enable_irq_mask(a_crtc, false,
			DPU_INT_BASE_ADDR + DPU_ONLINE_IRQ_MSK,
			saturn_hee_get_irq_bit(INT_UNDERRUN, dev_id));
		a_crtc->ur_mask_armed = false;
	}
}

void saturn_hee_enable_cmdlist(struct spacemit_crtc *a_crtc, struct spacemit_hw_device *hwdev,
			       int id, bool enable)
{
	u32 val = 0;
	u32 base = 0;
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_crtc_state *crtc_state = crtc->state;
	struct spacemit_crtc_state *spacemit_state =
			to_spacemit_crtc_state(crtc_state);
	struct spacemit_crtc_rdma *rdmas = spacemit_state->rdmas;

	base = DPU_SCENE_CTL_ADDR(a_crtc->dev_id);

	dpu_write_reg_arr(hwdev, DPU_CTL_TOP_REG, DPU_CTRL_BASE_ADDR,
			  dpu_ctl_top_reg_0, id, cmdlist_rch_en,
			  enable ? 1 : 0);
	val = dpu_read_reg(hwdev, DPU_CTL_REG, base, nml_rch_vrt_reuse);
	if (enable && rdmas[id].use_cnt > 1)
		val |= (1 << id);
	else
		val &= ~(1 << id);

	dpu_write(hwdev, DPU_CTL_REG, base, nml_rch_vrt_reuse, val);
}

void saturn_hee_cfg_cmdlist(struct spacemit_hw_device *hwdev, int id, u32 chy,
			    u32 addrl, u32 addrh)
{
	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR,
			  cmdlist_reg_48, id, cmdlist_ch_y_first, chy);
	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR,
			  cmdlist_reg_0, id, cmdlist_ch_start_addrl, addrl);
	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR,
			  cmdlist_reg_16, id, cmdlist_ch_start_addrh, addrh);
}

void saturn_hee_rdma_dmmu(struct spacemit_hw_device *hwdev, u8 tbu_id, struct tbu_instance *tbu,
			  struct drm_framebuffer *fb, u32 val, struct cmdlist_regs *cl_rdma,
			  struct drm_plane *plane)
{
	u32 base;
	u8 rdma_id = tbu_id / 2;
	struct cmdlist_regs *cl_tbu = NULL;

	SATURN_WRITE_RDMA_ADDR_REG(hwdev, 0, rdma_id, tbu->tbu_va[0], cl_rdma);
	SATURN_WRITE_RDMA_ADDR_REG(hwdev, 1, rdma_id, tbu->tbu_va[1], cl_rdma);
	SATURN_WRITE_RDMA_ADDR_REG(hwdev, 2, rdma_id, tbu->tbu_va[2], cl_rdma);

	base = RDMA_BASE_ADDR[rdma_id];
	dpu_write(hwdev, RDMA_PATH_X_REG, base, rdma_stride0_layer0,
		  fb->pitches[0], cl_rdma);
	dpu_write(hwdev, RDMA_PATH_X_REG, base, rdma_stride1_layer0,
		  fb->pitches[1], cl_rdma);

	cl_tbu = alloc_cmdlist_regs(MMU_TBU_REG);
	base = MMU_TBU_BASE_ADDR_ARRAY[tbu_id];
	SATURN_WRITE_TBU_REGS(hwdev, base, 0, cl_tbu);
	SATURN_WRITE_TBU_REGS(hwdev, base, 1, cl_tbu);
	SATURN_WRITE_TBU_REGS(hwdev, base, 2, cl_tbu);

	dpu_write_word(hwdev, base, 0, val, cl_tbu);
	cmdlist_regs_packing(plane_to_cl(plane), CMDLIST_MOD_DMMU, cl_tbu);
	free_cmdlist_regs(cl_tbu);
}

static u32 saturn_get_cl_chy_addr(void)
{
	return CMDLIST_BASE_ADDR + CMDLIST_CH_Y;
}

static u32 saturn_get_cl_start_cmps_y_addr(void)
{
	return DPU_CTRL_BASE_ADDR + CMDLIST_CH_START_CMPS_Y;
}

static u32 saturn_get_cl_cfg_rdy_addr(void)
{
	return DPU_CTRL_BASE_ADDR + CMDLIST_CFG_READY;
}

int saturn_hee_get_cl_rdma_buf(struct spacemit_crtc *a_crtc)
{
	a_crtc->cl_rdma = alloc_cmdlist_regs(RDMA_PATH_X_REG);
	if (!a_crtc->cl_rdma)
		return -ENOMEM;
	else
		return 0;
}

struct cmdlist_header {
	u64 next_list_addr : 39;
	/* reserved */
	u32: 1;
	u32 nod_len : 16;
	/* 1: the last cmdlist node, 2: pending node, otherwise 0 */
	u32 nod_type : 2;
	u32 next_nod_secu : 1;
	u64 wait_event_low    : 5;
	u64 wait_event    : 59;
	/* reserved */
	u32: 5;
};

struct cmdlist_row {
	u32 module_cfg_addr   : 19;
	u32 module_cfg_strobe : 12;
	/* the last row tag = 1, else tag = 0. */
	u32 row_eof_tag       : 1;
	u32 module_regs[3];
};

/* DMA stream layout consumed by the cmdlist engine; sizes are load-bearing. */
static_assert(sizeof(struct cmdlist_header) == 16);
static_assert(sizeof(struct cmdlist_row) == 16);

#define CL_HEADER_SZ sizeof(struct cmdlist_header)
#define CL_ROW_SZ    sizeof(struct cmdlist_row)

#define CMDLIST_RDMA_CFG_RDY(ch) BIT(ch)
#define CMDLIST_CMPS_CFG_RDY(ch) BIT((ch) + 20)

#define CMDLIST_WAIT_EVENT_BIT_RDMA_RELOAD BIT(4)
#define CMDLIST_WAIT_EVENT_BIT_CMPS_RELOAD BIT(6)

void saturn_hee_cmdlist_fill_data_row(struct cmdlist *cl, u32 strobe,
				      u32 offset, u32 value[])
{
	struct cmdlist_row *row;
	u8 i;

	/*
	 * packing counts rows blindly; drop the row rather than the buffer end
	 */
	if (WARN_ONCE(CL_HEADER_SZ + (cl->nod_len + 1) * CL_ROW_SZ > cl->size,
		      "cmdlist buffer full (rows %u)\n", cl->nod_len))
		return;

	row = (struct cmdlist_row *)((char *)cl->va + CL_HEADER_SZ) + cl->nod_len;
	row->module_cfg_addr = offset >> 2;
	row->module_cfg_strobe = strobe;

	for (i = 0; i < CMDLIST_ROW_REGS; i++)
		if (strobe & CMDLIST_REG_STROBE(i))
			row->module_regs[i] = value[i];
}

void saturn_hee_cmdlist_fill_conf_row(struct cmdlist *cl, struct spacemit_hw_device *hwdev,
				      u8 dev_id)
{
	struct cmdlist_header *header;
	struct cmdlist_row *row;
	struct spacemit_plane_state *spacemit_pstate = NULL;
	u8 rch_id = 0;
	u32 wait_event_bits = 0, cfg_rdy_bits = 0;
	int cur_crtc_y = -1, cur_crtc_h = -1, next_crtc_y = -1;

	/*
	 * CRTC-level cmdlists live inside a crtc_state; only planes have one.
	 */
	if (cl->type == CMDLIST_PLANE) {
		spacemit_pstate = cl_to_spacemit_pstate(cl);
		rch_id = spacemit_pstate->rdma_id;
	}

	/* At most three rows are appended below; bail before overrunning. */
	if (WARN_ONCE(CL_HEADER_SZ + (cl->nod_len + 3) * CL_ROW_SZ > cl->size,
		      "cmdlist buffer full (type %u, rows %u)\n",
		      cl->type, cl->nod_len))
		return;

	header = (struct cmdlist_header *)(cl->va);
	row = (struct cmdlist_row *)((char *)header + CL_HEADER_SZ + cl->nod_len * CL_ROW_SZ);
	if (cl->next) {
		if (dev_id == CMDLIST_CMP_INVALID) {
			trace_u64_data("rch_id", rch_id);
			row->module_cfg_addr = ((saturn_get_cl_chy_addr() + rch_id * 4) >> 2);
		} else {
			trace_u64_data("dev_id", dev_id);
			row->module_cfg_addr = ((saturn_get_cl_chy_addr() + (5 + dev_id) * 4) >> 2);
		}
		row->module_cfg_strobe = CMDLIST_REG_STROBE(0);
		if (cl->cmdlist_ch_y_other != CMDLIST_SENTINEL_POISON)
			next_crtc_y = cl->cmdlist_ch_y_other;
		else if (cl->next->type == CMDLIST_PLANE)
			next_crtc_y = cl_to_spacemit_pstate(cl->next)->state.crtc_y;
		else
			next_crtc_y = 0;
		row->module_regs[0] = next_crtc_y;
		cl->nod_len++;
		row++;
	}

	if (cl->type == CMDLIST_PLANE) {
		if (cl->rch_start_cmps_y != CMDLIST_SENTINEL_POISON)
			cur_crtc_y = cl->rch_start_cmps_y;
		else
			cur_crtc_y = cl_to_spacemit_pstate(cl)->state.crtc_y;
		cur_crtc_h = cl_to_spacemit_pstate(cl)->state.crtc_h;
		/* the cmdlist nodes don't belong to the same plane */
		if (next_crtc_y != -1 && cl_to_spacemit_pstate(cl->next) != spacemit_pstate &&
		    (cur_crtc_y + cur_crtc_h + 24) > next_crtc_y) {
			drm_err_ratelimited(spacemit_pstate->state.crtc->dev,
				"invalid gap: cur_y=%d cur_h=%d next_y=%d\n",
				cur_crtc_y, cur_crtc_h, next_crtc_y);
		}
		row->module_cfg_addr = (saturn_get_cl_start_cmps_y_addr() + rch_id * 4) >> 2;
		row->module_cfg_strobe = CMDLIST_REG_STROBE(0);
		row->module_regs[0] =
				(cur_crtc_y >= 24) ? (cur_crtc_y - 24) : 0;
		cl->nod_len++;
		row++;
	}

	if (cl->mode_mask & CMDLIST_MOD_RDMA) {
		wait_event_bits |= CMDLIST_WAIT_EVENT_BIT_RDMA_RELOAD;
		cfg_rdy_bits |= CMDLIST_RDMA_CFG_RDY(rch_id);
	}
	if (cl->mode_mask & CMDLIST_MOD_COMP) {
		wait_event_bits |= CMDLIST_WAIT_EVENT_BIT_CMPS_RELOAD;
		cfg_rdy_bits |= CMDLIST_CMPS_CFG_RDY(dev_id);
	}
	row->module_cfg_addr = (saturn_get_cl_cfg_rdy_addr()) >> 2;
	row->module_cfg_strobe = CMDLIST_REG_STROBE(0);
	row->module_regs[0] = cfg_rdy_bits;
	row->row_eof_tag = 1;
	cl->nod_len++;
	row++;

	if (cl->next)
		header->next_list_addr = cl->next->pa;
	else
		header->nod_type = 1;
	header->nod_len = cl->nod_len + 1; /* include header */
	header->wait_event_low = wait_event_bits & 0x1F;
	header->wait_event = wait_event_bits >> 5;
}

void saturn_hee_crtc_cmdlist(struct spacemit_hw_device *hwdev,
			     struct spacemit_drm_private *priv, u8 crtc_id, u8 dev_id)
{
	u32 val = 0;

	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_48,
			  hwdev->rdma_nums + 2 + crtc_id, cmdlist_ch_y_first,
			  val);

	val = (lower_32_bits(priv->cmdlist_groups[hwdev->rdma_nums + crtc_id]->pa) &
	       CMDLIST_ADDRL_ALIGN_MASK) >> CMDLIST_ADDRL_ALIGN_BITS;
	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_0,
			  hwdev->rdma_nums + 2 + crtc_id,
			  cmdlist_ch_start_addrl, val);
	val = upper_32_bits(priv->cmdlist_groups[hwdev->rdma_nums + crtc_id]->pa);
	dpu_write_reg_arr(hwdev, CMDLIST_REG, CMDLIST_BASE_ADDR, cmdlist_reg_16,
			  hwdev->rdma_nums + 2 + crtc_id,
			  cmdlist_ch_start_addrh, val);
	dpu_write_reg_arr(hwdev, DPU_CTL_TOP_REG, DPU_CTRL_BASE_ADDR,
			  dpu_ctl_top_reg_10, dev_id, cmdlist_cmps_top_en, 1);
	dpu_write_reg_arr(hwdev, DPU_CTL_TOP_REG, DPU_CTRL_BASE_ADDR,
			  dpu_ctl_top_reg_10, dev_id, cmdlist_cmps_other_en, 1);

	priv->cmdlist_groups[hwdev->rdma_nums + crtc_id] = NULL;
}

#define DUMP_BUF_LEN	100
void saturn_hee_cmdlist_dump_node(struct cmdlist *cl)
{
	struct cmdlist_header *header = (struct cmdlist_header *)(cl->va);
	ssize_t ret = 0;
	char tmp[DUMP_BUF_LEN] = {0x0};

	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "header: ");
	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "next_addr:0x%llx ",
			 (unsigned long long)header->next_list_addr);
	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "rows:%d ",
			 header->nod_len);
	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "type:%d ",
			 header->nod_type);
	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "event_low:0x%llx ",
			 (unsigned long long)header->wait_event_low);
	ret += scnprintf(tmp + ret, sizeof(tmp) - ret, "event:0x%llx ",
			 (unsigned long long)header->wait_event);
	trace_plat_cmdlist_dump_node(tmp);

	for (int i = 0; i < header->nod_len - 1; i++) {
		struct cmdlist_row *row = (struct cmdlist_row *)((char *)header + CL_HEADER_SZ) + i;
		u32 addr = row->module_cfg_addr << 2;

		ret = 0;
		ret += scnprintf(tmp + ret, sizeof(tmp) - ret,
				 "row[%4d:tag:%d]: ", i, row->row_eof_tag);

		for (int j = 0; j < CMDLIST_ROW_REGS; j++) {
			if (row->module_cfg_strobe & CMDLIST_REG_STROBE(j))
				ret += scnprintf(tmp + ret, sizeof(tmp) - ret,
						 "[%#10x:%#10x]", addr + j * 4,
						 row->module_regs[j]);
			else
				ret += scnprintf(tmp + ret, sizeof(tmp) - ret,
						 "%#11x:%#11x", addr + j * 4,
						 row->module_regs[j]);
		}
		trace_plat_cmdlist_dump_node(tmp);
	}
}

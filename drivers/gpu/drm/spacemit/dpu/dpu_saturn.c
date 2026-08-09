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
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/types.h>
#include <linux/math64.h>
#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem.h>
#include "dpu_saturn.h"
#include "saturn_fbcmem.h"
#include "../spacemit_cmdlist.h"
#include "../spacemit_dmmu.h"
#include "../spacemit_dpu_reg.h"
#include "../spacemit_drm.h"
#include <video/display_timing.h>
#include "../spacemit_dpu_ids.h"
#include "saturn_regs/ops_hee.h"

#define CREATE_TRACE_POINTS
#include "dpu_trace.h"

#define TOTAL_RDMA_MEMSIZE	(68 * 1024)
static const struct dpu_format_id primary_fmts[] = {
	{ DRM_FORMAT_XRGB8888, 8, 32 }, /* RDMA_FMT_XRGB_8888 */
	{ DRM_FORMAT_ARGB8888, 4, 32 }, /* RDMA_FMT_ARGB_8888 */
	{ DRM_FORMAT_XBGR8888, 9, 32 }, /* RDMA_FMT_XBGR_8888 */
	{ DRM_FORMAT_ABGR8888, 5, 32 }, /* RDMA_FMT_ABGR_8888 */
	{ DRM_FORMAT_RGBX8888, 10, 32 }, /* RDMA_FMT_RGBX_8888 */
	{ DRM_FORMAT_RGBA8888, 6, 32 }, /* RDMA_FMT_RGBA_8888 */
	{ DRM_FORMAT_BGRX8888, 11, 32 }, /* RDMA_FMT_BGRX_8888 */
	{ DRM_FORMAT_BGRA8888, 7, 32 }, /* RDMA_FMT_BGRA_8888 */
	{ DRM_FORMAT_ABGR2101010, 1, 32 }, /* RDMA_FMT_ABGR_2101010 */
	{ DRM_FORMAT_RGB888, 12, 24 }, /* RDMA_FMT_RGB_888 */
	{ DRM_FORMAT_BGR888, 13, 24 }, /* RDMA_FMT_BGR_888 */
	{ DRM_FORMAT_RGB565, 22, 16 }, /* RDMA_FMT_RGB_565 */
	{ DRM_FORMAT_BGR565, 23, 16 }, /* RDMA_FMT_BGR_565 */
};

static const struct spacemit_hw_rdma saturn_hee_rdmas[] = {
	{FORMAT_RGB},
	{FORMAT_RGB},
	{FORMAT_RGB},
	{FORMAT_RGB},
};

static const u32 saturn_hee_fbcmem_sizes[] = {
	89600, /* 87.5k */
	15360, /* 16k */
};

const struct spacemit_hw_device k3_saturn_dpu = {
	.plane_nums = 1,
	.crtc_nums = 2,
	.rdma_nums = ARRAY_SIZE(saturn_hee_rdmas),
	.rdmas = saturn_hee_rdmas,
	.n_formats = ARRAY_SIZE(primary_fmts),
	.formats = primary_fmts,
	.n_fbcmems = ARRAY_SIZE(saturn_hee_fbcmem_sizes),
	.fbcmem_sizes = saturn_hee_fbcmem_sizes,
	.enable_vsync = saturn_hee_enable_vsync,
	.enable_cfg_irq = saturn_hee_enable_cfg_irq,
	.cfg_ready = saturn_hee_cfg_ready,
	.sw_start = saturn_hee_sw_start,
	.irq_enable = saturn_hee_irq_enable,
	.dpu_init = saturn_hee_dpu_init,
	.plane_update_hw_channel = saturn_hee_plane_update_hw_channel,
	.plane_disable_hw_channel = saturn_hee_plane_disable_hw_channel,
	.conf_dpuctrl = saturn_hee_conf_dpuctrl,
	.get_cfg_rdy = saturn_hee_get_cfg_rdy,
	.get_int_sts = saturn_hee_get_int_sts,
	.get_irq_bit = saturn_hee_get_irq_bit,
	.clr_int_sts = saturn_hee_clr_int_sts,
	.enable_cmdlist = saturn_hee_enable_cmdlist,
	.cfg_cmdlist = saturn_hee_cfg_cmdlist,
	.rdma_dmmu = saturn_hee_rdma_dmmu,
	.get_cl_rdma_buf = saturn_hee_get_cl_rdma_buf,
	.cmdlist_fill_data_row = saturn_hee_cmdlist_fill_data_row,
	.cmdlist_fill_conf_row = saturn_hee_cmdlist_fill_conf_row,
	.crtc_cmdlist = saturn_hee_crtc_cmdlist,
	.cmdlist_dump_node = saturn_hee_cmdlist_dump_node,
};

void saturn_enable_irq_mask(struct spacemit_crtc *a_crtc, bool enable,
			    u32 offset, u32 mask)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	if (!hwdev->base) {
		drm_err(a_crtc->crtc.dev, "hwdev->base is NULL\n");
		return;
	}

	if (!a_crtc->clocks_on)
		return;

	regmap_update_bits(hwdev->regmap, offset, mask, enable ? mask : 0);
}

int dpu_parse_clocks(struct device *dev, struct dpu_clk_context *clk_ctx)
{
	clk_ctx->pxclk = devm_clk_get(dev, "pxclk");
	if (IS_ERR(clk_ctx->pxclk))
		return dev_err_probe(dev, PTR_ERR(clk_ctx->pxclk),
				     "failed to get pxclk\n");

	clk_ctx->mclk = devm_clk_get(dev, "mclk");
	if (IS_ERR(clk_ctx->mclk))
		return dev_err_probe(dev, PTR_ERR(clk_ctx->mclk),
				     "failed to get mclk\n");

	clk_ctx->escclk = devm_clk_get(dev, "escclk");
	if (IS_ERR(clk_ctx->escclk))
		return dev_err_probe(dev, PTR_ERR(clk_ctx->escclk),
				     "failed to get escclk\n");

	clk_ctx->aclk = devm_clk_get(dev, "aclk");
	if (IS_ERR(clk_ctx->aclk))
		return dev_err_probe(dev, PTR_ERR(clk_ctx->aclk),
				     "failed to get aclk\n");

	clk_ctx->dscclk = devm_clk_get(dev, "dscclk");
	if (IS_ERR(clk_ctx->dscclk))
		return dev_err_probe(dev, PTR_ERR(clk_ctx->dscclk),
				     "failed to get dscclk\n");

	return 0;
}

static int dpu_enable_clocks(struct spacemit_crtc *a_crtc)
{
	struct dpu_clk_context *clk_ctx = &a_crtc->clk_ctx;
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_display_mode *mode = &crtc->mode;
	u64 clk_val;
	u64 set_clk_val;
	int ret;

	ret = clk_prepare_enable(clk_ctx->pxclk);
	if (ret)
		goto err;
	set_clk_val = mode->clock * 1000;
	if (set_clk_val) {
		set_clk_val = clk_round_rate(clk_ctx->pxclk, set_clk_val);
		clk_val = clk_get_rate(clk_ctx->pxclk);
		if (clk_val != set_clk_val) {
			clk_set_rate(clk_ctx->pxclk, set_clk_val);
			drm_dbg(crtc->dev, "pxclk=%lld\n", clk_val);
		}
	}

	ret = clk_prepare_enable(clk_ctx->mclk);
	if (ret)
		goto err_pxclk;
	clk_val = clk_get_rate(clk_ctx->mclk);
	if (clk_val != DPU_MCLK_DEFAULT) {
		clk_val = clk_round_rate(clk_ctx->mclk, DPU_MCLK_DEFAULT);
		clk_set_rate(clk_ctx->mclk, clk_val);
		drm_dbg(crtc->dev, "mclk=%lld\n", clk_val);
	}

	/* The escape clock only has to run for the "esc" reset to propagate. */
	ret = clk_prepare_enable(clk_ctx->escclk);
	if (ret)
		goto err_mclk;

	ret = clk_prepare_enable(clk_ctx->aclk);
	if (ret)
		goto err_escclk;
	clk_val = clk_get_rate(clk_ctx->aclk);
	if (clk_val != a_crtc->aclk) {
		set_clk_val = clk_round_rate(clk_ctx->aclk, a_crtc->aclk);
		if (set_clk_val != clk_val) {
			clk_set_rate(clk_ctx->aclk, set_clk_val);
			drm_dbg(crtc->dev, "aclk %llu -> %llu\n", clk_val,
				set_clk_val);
		}
	}

	ret = clk_prepare_enable(clk_ctx->dscclk);
	if (ret)
		goto err_aclk;

	trace_dpu_enable_clocks(a_crtc->dev_id);
	return 0;

err_aclk:
	clk_disable_unprepare(clk_ctx->aclk);
err_escclk:
	clk_disable_unprepare(clk_ctx->escclk);
err_mclk:
	clk_disable_unprepare(clk_ctx->mclk);
err_pxclk:
	clk_disable_unprepare(clk_ctx->pxclk);
err:
	drm_err(crtc->dev, "failed to enable DPU clocks: %d\n", ret);
	return ret;
}

static int dpu_disable_clocks(struct spacemit_crtc *a_crtc)
{
	struct dpu_clk_context *clk_ctx = &a_crtc->clk_ctx;

	trace_dpu_disable_clocks(a_crtc->dev_id);

	clk_disable_unprepare(clk_ctx->pxclk);
	clk_disable_unprepare(clk_ctx->mclk);
	clk_disable_unprepare(clk_ctx->escclk);

	clk_disable_unprepare(clk_ctx->aclk);
	clk_disable_unprepare(clk_ctx->dscclk);

	return 0;
}

u8 spacemit_plane_hw_get_format_id(u32 format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(primary_fmts); i++) {
		if (primary_fmts[i].format == format)
			return primary_fmts[i].id;
	}

	return SPACEMIT_DPU_INVALID_FORMAT_ID;
}

static void saturn_enable_vsync(struct spacemit_crtc *a_crtc, bool enable)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	trace_saturn_enable_vsync("vsync", enable);
	hwdev->enable_vsync(a_crtc, hwdev, enable);
}

void spacemit_cfg_rdy_timer_handler(struct timer_list *t)
{
	struct spacemit_crtc *a_crtc = timer_container_of(a_crtc, t,
							  cfg_rdy_timer);
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_device *drm = crtc->dev;
	struct drm_pending_vblank_event *event;
	unsigned long flags;

	drm_err_ratelimited(drm, "CFG_RDY handshake timeout\n");
	trace_saturn_ctrl_cfg_ready_timer(a_crtc->dev_id);

	a_crtc->flip_done = true;

	spin_lock_irqsave(&drm->event_lock, flags);
	event = crtc->state->event;
	if (event) {
		crtc->state->event = NULL;
		drm_crtc_send_vblank_event(crtc, event);
	}
	spin_unlock_irqrestore(&drm->event_lock, flags);

	drm_crtc_vblank_put(crtc);
}

static void saturn_ctrl_cfg_ready(struct spacemit_crtc *a_crtc, bool enable)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	trace_saturn_ctrl_cfg_ready(a_crtc->dev_id, enable);
	mod_timer(&a_crtc->cfg_rdy_timer, jiffies + msecs_to_jiffies(3000));
	hwdev->cfg_ready(a_crtc, hwdev);
}

static void saturn_ctrl_sw_start(struct spacemit_crtc *a_crtc, bool enable)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	trace_saturn_ctrl_sw_start(a_crtc->dev_id, enable);

	hwdev->sw_start(a_crtc, hwdev);
}

static int dpu_init(struct spacemit_crtc *a_crtc)
{
	unsigned int timeout = 1000;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	if (!a_crtc->clocks_on)
		return 0;

	while (timeout) {
		if (hwdev->get_cfg_rdy(a_crtc, hwdev) == 0)
			break;
		usleep_range(100, 200);
		timeout--;
	}
	if (timeout == 0)
		drm_err(a_crtc->crtc.dev, "wait for cfg ready timed out\n");

	hwdev->dpu_init(a_crtc);

	return 0;
}

static void dpu_uninit(struct spacemit_crtc *a_crtc)
{
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	if (!a_crtc->clocks_on)
		return;

	trace_dpu_uninit(a_crtc->dev_id);
	hwdev->irq_enable(a_crtc, false);
}

static inline void dpu_isr_vblank(struct spacemit_crtc *a_crtc, bool *flip)
{
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_device *drm = crtc->dev;
	struct drm_pending_vblank_event *event;

	drm_crtc_handle_vblank(crtc);

	if (*flip)
		return;
	*flip = true;

	/*
	 * event_lock serialises this against the commit arming
	 * crtc->state->event.
	 */
	spin_lock(&drm->event_lock);
	event = crtc->state->event;
	if (event) {
		/*
		 * Consume it exactly once, even against a concurrent
		 * commit_hw_done().
		 */
		crtc->state->event = NULL;
		drm_crtc_send_vblank_event(crtc, event);
	}
	spin_unlock(&drm->event_lock);

	drm_crtc_vblank_put(crtc);
}

u32 saturn_conf_dpuctrl_rdma(struct spacemit_crtc *a_crtc)
{
	struct drm_crtc *crtc = &a_crtc->crtc;
	struct drm_plane *plane;
	u32 rdma_en = 0;

	drm_atomic_crtc_for_each_plane(plane, crtc) {
		u32 rdma_id = to_spacemit_plane_state(plane->state)->rdma_id;

		if (rdma_id != RDMA_INVALID_ID)
			rdma_en |= (1 << rdma_id);
	}

	trace_dpuctrl("rdma_en", rdma_en);

	return rdma_en;
}

static u32 dpu_online_isr(struct spacemit_crtc *a_crtc)
{
	u32 irq_raw, irq_bit, irq_ur_bit;
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	int dev_id = a_crtc->dev_id;

	trace_dpu_isr(a_crtc->dev_id);

	irq_raw = hwdev->get_int_sts(hwdev, dev_id);
	trace_dpu_isr_status("ONLINE", irq_raw);
	irq_bit = hwdev->get_irq_bit(INT_UNDERRUN, dev_id);
	irq_ur_bit = irq_raw & irq_bit;
	if (irq_ur_bit && !a_crtc->ur_reported) {
		hwdev->clr_int_sts(a_crtc, irq_ur_bit, dev_id);
		trace_dpu_isr_status("underrun", irq_ur_bit);
		drm_err_ratelimited(a_crtc->crtc.dev, "underrun\n");
		a_crtc->ur_reported = true;
	}
	irq_bit = hwdev->get_irq_bit(INT_CFG_RDY, dev_id);
	if (irq_raw & irq_bit) {
		hwdev->clr_int_sts(a_crtc, irq_bit, dev_id);
		trace_dpu_isr_status("cfg_rdy_clr", irq_raw & irq_bit);
		/*
		 * A fired watchdog already sent the event and put this flip's
		 * vblank reference; re-arming the vsync path would put twice.
		 */
		if (timer_delete(&a_crtc->cfg_rdy_timer))
			a_crtc->flip_done = false;
		a_crtc->ur_reported = false;
		if (hwdev->enable_cfg_irq)
			hwdev->enable_cfg_irq(a_crtc, hwdev, false);
	}
	irq_bit = hwdev->get_irq_bit(INT_VSYNC, dev_id);
	if (irq_raw & irq_bit) {
		hwdev->clr_int_sts(a_crtc, irq_bit, dev_id);
		trace_dpu_isr_status("vsync", irq_raw & irq_bit);
		dpu_isr_vblank(a_crtc, &a_crtc->flip_done);
	}
	irq_bit = hwdev->get_irq_bit(INT_REST, dev_id) | irq_ur_bit;
	if (irq_raw & irq_bit)
		hwdev->clr_int_sts(a_crtc, irq_bit, dev_id);

	return 0;
}

static void dpu_run(struct drm_crtc *crtc,
		    struct drm_crtc_state *old_state)
{
	struct spacemit_crtc *a_crtc = to_spacemit_crtc(crtc);
	struct spacemit_drm_private *priv = a_crtc->crtc.dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;

	trace_dpu_run(a_crtc->dev_id);

	if (hwdev->conf_dpuctrl)
		hwdev->conf_dpuctrl(crtc, old_state);

	/* flush all config register writes before CFG_RDY is raised */
	mb();

	drm_crtc_vblank_get(crtc);
	if (hwdev->enable_cfg_irq)
		hwdev->enable_cfg_irq(a_crtc, hwdev, true);

	saturn_ctrl_cfg_ready(a_crtc, true);

	if (unlikely(a_crtc->is_1st_f)) {
		drm_dbg(a_crtc->crtc.dev, "DPU start\n");
		a_crtc->is_1st_f = false;
		saturn_ctrl_sw_start(a_crtc, true);
	}
	dma_rmb();
}

static void dpu_enable_vsync(struct spacemit_crtc *a_crtc)
{
	saturn_enable_vsync(a_crtc, true);
}

static void dpu_disable_vsync(struct spacemit_crtc *a_crtc)
{
	saturn_enable_vsync(a_crtc, false);
}

const struct dpu_core_ops dpu_saturn_ops = {
	.init = dpu_init,
	.uninit = dpu_uninit,
	.run = dpu_run,
	.online_isr = dpu_online_isr,
	.enable_clk = dpu_enable_clocks,
	.disable_clk = dpu_disable_clocks,
	.enable_vsync = dpu_enable_vsync,
	.disable_vsync = dpu_disable_vsync,
	.cal_layer_fbcmem_size = saturn_cal_layer_fbcmem_size,
	.adjust_rdma_fbcmem = saturn_adjust_rdma_fbcmem,
};

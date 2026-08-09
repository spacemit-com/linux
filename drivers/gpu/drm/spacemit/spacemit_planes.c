// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_blend.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_managed.h>
#include <drm/drm_plane.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_color_mgmt.h>
#include <drm/drm_gem.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include "spacemit_cmdlist.h"
#include "spacemit_dmmu.h"
#include "spacemit_drm.h"
#include "dpu/dpu_saturn.h"
#include "dpu/dpu_trace.h"
#include "dpu/saturn_fbcmem.h"

/* The AFBC decoder exists but no modifier has been validated on hardware. */
static const u64 supported_format_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID
};

struct spacemit_plane *to_spacemit_plane(struct drm_plane *plane)
{
	return container_of(plane, struct spacemit_plane, plane);
}

static void spacemit_plane_atomic_destroy_state(struct drm_plane *plane,
						struct drm_plane_state *state);

static int spacemit_plane_atomic_check(struct drm_plane *plane,
				       struct drm_atomic_commit *atomic_state)
{
	struct drm_plane_state *state =
			drm_atomic_get_new_plane_state(atomic_state, plane);
	struct drm_framebuffer *fb = state->fb;
	u32 src_w, src_h, src_x, src_y;
	u32 crtc_w, crtc_h;
	struct spacemit_plane_state *cur_state = to_spacemit_plane_state(state);
	u32 cur_rdma_id = cur_state->rdma_id;
	struct spacemit_drm_private *priv = plane->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct spacemit_crtc *a_crtc = NULL;

	struct drm_crtc_state *crtc_state = NULL;
	int ret;

	if (state->crtc)
		crtc_state = drm_atomic_get_new_crtc_state(atomic_state,
							   state->crtc);

	ret = drm_atomic_helper_check_plane_state(state, crtc_state,
						  DRM_PLANE_NO_SCALING,
						  DRM_PLANE_NO_SCALING,
						  true, false);
	if (ret)
		return ret;

	if (!state->visible)
		return 0;

	a_crtc = to_spacemit_crtc(state->crtc);
	trace_spacemit_plane_atomic_check(a_crtc->dev_id);

	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;
	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;
	crtc_w = state->crtc_w;
	crtc_h = state->crtc_h;

	/* The programming paths use the raw rectangles, so refuse to clip. */
	if ((state->src.x1 >> 16) != src_x || (state->src.y1 >> 16) != src_y ||
	    (drm_rect_width(&state->src) >> 16) != src_w ||
	    (drm_rect_height(&state->src) >> 16) != src_h ||
	    state->dst.x1 != state->crtc_x || state->dst.y1 != state->crtc_y ||
	    drm_rect_width(&state->dst) != crtc_w ||
	    drm_rect_height(&state->dst) != crtc_h) {
		drm_dbg_kms(plane->dev,
			    "clipped plane rectangles not supported\n");
		return -EINVAL;
	}

	/* One-dimensional sources are invalid (only 0x0 = constant fill) */
	if ((src_w == 0) != (src_h == 0)) {
		drm_dbg_kms(plane->dev, "unsupported src_w %d src_h %d\n",
			    src_w, src_h);
		return -EINVAL;
	}

	if (src_w == 0 && src_h == 0)
		cur_rdma_id = RDMA_INVALID_ID;	/* constant-fill layer, no RDMA */
	else if (cur_rdma_id == RDMA_INVALID_ID)
		cur_rdma_id = state->zpos;	/* first commit: bind channel by zpos */
	cur_state->rdma_id = cur_rdma_id;

	if (cur_rdma_id != RDMA_INVALID_ID) {
		if (cur_rdma_id >= hwdev->rdma_nums) {
			drm_dbg_kms(plane->dev, "invalid rdma id %d\n",
				    cur_rdma_id);
			return -EINVAL;
		}

		if (a_crtc->core->cal_layer_fbcmem_size(plane, state)) {
			drm_dbg_kms(plane->dev,
				    "plane %d: invalid fbcmem size\n",
				    state->zpos);
			return -EINVAL;
		}

		cur_state->mmu_tbl.size =
			((PAGE_ALIGN(fb->obj[0]->size) >> PAGE_SHIFT) +
			 HW_ALIGN_TTB_NUM) * 4;
		cur_state->mmu_tbl.va =
			dma_alloc_coherent(a_crtc->dev, cur_state->mmu_tbl.size,
					   &cur_state->mmu_tbl.pa,
					   GFP_KERNEL | __GFP_ZERO);
		if (!cur_state->mmu_tbl.va)
			return -ENOMEM;
	}

	/*
	 * The commit cannot fail, so take the DMA buffer while -ENOMEM is an
	 * answer.
	 */
	ret = spacemit_cmdlist_alloc(a_crtc->dev, &cur_state->cl);
	if (ret)
		return ret;

	cur_state->format = spacemit_plane_hw_get_format_id(fb->format->format);
	if (cur_state->format == SPACEMIT_DPU_INVALID_FORMAT_ID) {
		drm_dbg_kms(plane->dev, "unsupported format %p4cc\n",
			    &fb->format->format);
		return -EINVAL;
	}

	return 0;
}

static void spacemit_plane_atomic_update(struct drm_plane *plane,
					 struct drm_atomic_commit *state)
{
	int ret = 0;
	struct spacemit_crtc *a_crtc = to_spacemit_crtc(plane->state->crtc);
	struct spacemit_plane_state *spacemit_pstate =
			to_spacemit_plane_state(plane->state);
	struct spacemit_drm_private *priv = plane->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	u32 rdma_id = spacemit_pstate->rdma_id;
	struct cmdlist *cl = NULL;
	int idx;

	if (!drm_dev_enter(plane->dev, &idx))
		return;

	trace_spacemit_plane_atomic_update(a_crtc->dev_id);

	hwdev->get_cl_rdma_buf(a_crtc);

	hwdev->plane_update_hw_channel(plane);

	/* Solid-fill layers have no RDMA and no page table */
	if (rdma_id < hwdev->rdma_nums && spacemit_pstate->mmu_tbl.va) {
		u8 tbu_id = rdma_id * 2;

		ret = spacemit_dmmu_map(plane->state->fb,
					&spacemit_pstate->mmu_tbl, tbu_id,
					a_crtc->cl_rdma, plane);
		if (!ret)
			cmdlist_regs_packing(plane_to_cl(plane),
					     CMDLIST_MOD_RDMA, a_crtc->cl_rdma);
		else
			drm_err(plane->dev, "failed to map plane: %d\n", ret);
	}

	cl = &spacemit_pstate->cl;
	cl->rch_start_cmps_y = CMDLIST_SENTINEL_POISON;
	cl->cmdlist_ch_y_other = CMDLIST_SENTINEL_POISON;
	free_cmdlist_regs(a_crtc->cl_rdma);

	drm_dev_exit(idx);
}

static void spacemit_plane_atomic_disable(struct drm_plane *plane,
					  struct drm_atomic_commit *state)
{
	struct drm_plane_state *old_state =
			drm_atomic_get_old_plane_state(state, plane);
	struct spacemit_drm_private *priv = plane->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	int idx;

	if (!drm_dev_enter(plane->dev, &idx))
		return;

	hwdev->plane_disable_hw_channel(plane, old_state);

	drm_dev_exit(idx);
}

static void spacemit_plane_reset(struct drm_plane *plane)
{
	struct spacemit_plane_state *s;

	if (plane->state) {
		spacemit_plane_atomic_destroy_state(plane, plane->state);
		plane->state = NULL;
	}

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (s) {
		__drm_atomic_helper_plane_reset(plane, &s->state);
		s->rdma_id = RDMA_INVALID_ID;
	}
}

static struct drm_plane_state *
spacemit_plane_atomic_duplicate_state(struct drm_plane *plane)
{
	struct spacemit_plane_state *s;
	struct spacemit_plane_state *old_state =
			to_spacemit_plane_state(plane->state);
	struct spacemit_crtc *a_crtc = NULL;

	if (plane->crtc) {
		a_crtc = to_spacemit_crtc(plane->crtc);
		trace_spacemit_plane_atomic_duplicate_state(a_crtc->dev_id);
	}

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &s->state);

	WARN_ON(s->state.plane != plane);

	s->cl.type = CMDLIST_PLANE;

	s->rdma_id = old_state->rdma_id;
	s->format = old_state->format;
	s->fbcmem_size = 0;

	return &s->state;
}

static void spacemit_plane_atomic_destroy_state(struct drm_plane *plane,
						struct drm_plane_state *state)
{
	struct spacemit_plane_state *spacemit_pstate =
			to_spacemit_plane_state(state);
	struct spacemit_crtc *a_crtc = NULL;

	if (state->crtc) {
		a_crtc = to_spacemit_crtc(state->crtc);

		if (spacemit_pstate->mmu_tbl.va)
			dma_free_coherent(a_crtc->dev,
					  spacemit_pstate->mmu_tbl.size,
					  spacemit_pstate->mmu_tbl.va,
					  spacemit_pstate->mmu_tbl.pa);

		if (spacemit_pstate->cl.va)
			dma_free_coherent(a_crtc->dev,
					  spacemit_pstate->cl.size,
					  spacemit_pstate->cl.va,
					  spacemit_pstate->cl.pa);
		trace_spacemit_plane_atomic_destroy_state(a_crtc->dev_id);
	}
	__drm_atomic_helper_plane_destroy_state(state);

	kfree(to_spacemit_plane_state(state));
}

static void spacemit_plane_create_properties(struct spacemit_plane *p)
{
	unsigned int support_modes = BIT(DRM_MODE_BLEND_PIXEL_NONE) |
		BIT(DRM_MODE_BLEND_PREMULTI) |
		BIT(DRM_MODE_BLEND_COVERAGE);

	drm_plane_create_alpha_property(&p->plane);

	drm_plane_create_blend_mode_property(&p->plane, support_modes);
}

static const struct drm_plane_helper_funcs spacemit_plane_helper_funcs = {
	.atomic_check = spacemit_plane_atomic_check,
	.atomic_update = spacemit_plane_atomic_update,
	.atomic_disable = spacemit_plane_atomic_disable,
};

static const struct drm_plane_funcs spacemit_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = spacemit_plane_reset,
	.atomic_duplicate_state = spacemit_plane_atomic_duplicate_state,
	.atomic_destroy_state = spacemit_plane_atomic_destroy_state,
};

struct drm_plane *spacemit_plane_init(struct drm_device *drm,
				      struct spacemit_crtc *a_crtc)
{
	struct spacemit_plane *p = NULL;
	int err, j;
	u32 *formats;
	struct spacemit_drm_private *priv = drm->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	u8 n_formats = hwdev->n_formats;
	u8 n_fbcmems = hwdev->n_fbcmems;
	u8 n_rdmas = hwdev->rdma_nums;
	u32 plane_crtc_mask;

	trace_spacemit_plane_init(a_crtc->dev_id);
	if (n_fbcmems * 2 != n_rdmas) {
		drm_err(drm, "mismatched rdma/fbcmem counts: %d vs %d\n",
			n_rdmas, n_fbcmems);
		return ERR_PTR(-EINVAL);
	}

	for (j = 0; j < n_fbcmems; j++) {
		if (hwdev->fbcmem_sizes[j] < FBCMEM_UNIT) {
			drm_err(drm, "invalid fbcmem_sizes[%d] = %u\n",
				j, hwdev->fbcmem_sizes[j]);
			return ERR_PTR(-EINVAL);
		}
	}

	formats = kcalloc(n_formats, sizeof(*formats), GFP_KERNEL);
	if (!formats)
		return ERR_PTR(-ENOMEM);

	/*
	 * The CRTC is created right after this, so its index is the current
	 * count.
	 */
	plane_crtc_mask = 1 << drm->mode_config.num_crtc;

	/* drmm, not devm: an open DRM fd outlives the platform unbind. */
	p = drmm_kzalloc(drm, sizeof(*p), GFP_KERNEL);
	if (!p) {
		kfree(formats);
		return ERR_PTR(-ENOMEM);
	}

	for (j = 0; j < n_formats; j++)
		formats[j] = hwdev->formats[j].format;

	err = drm_universal_plane_init(drm, &p->plane, plane_crtc_mask,
				       &spacemit_plane_funcs, formats,
				       n_formats, supported_format_modifiers,
				       DRM_PLANE_TYPE_PRIMARY, NULL);
	kfree(formats);
	if (err) {
		drm_err(drm, "failed to init primary plane\n");
		return ERR_PTR(err);
	}

	drm_plane_helper_add(&p->plane, &spacemit_plane_helper_funcs);

	spacemit_plane_create_properties(p);

	p->hwdev = hwdev;

	return &p->plane;
}

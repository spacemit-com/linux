/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM spacemit_dpu

#if !defined(_DPU_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _DPU_TRACE_H_

#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

#include "dpu_saturn.h"

TRACE_EVENT(drm_display_mode_info,
	    TP_PROTO(struct drm_display_mode *mode),
	TP_ARGS(mode),
	TP_STRUCT__entry(
		__field(u16, hdisplay)
		__field(u16, hbp)
		__field(u16, hfp)
		__field(u16, hsync)
		__field(u16, vdisplay)
		__field(u16, vbp)
		__field(u16, vfp)
		__field(u16, vsync)
	),
	TP_fast_assign(
		__entry->hdisplay = mode->hdisplay;
		__entry->hbp = mode->htotal - mode->hsync_end;
		__entry->hfp = mode->hsync_start - mode->hdisplay;
		__entry->hsync = mode->hsync_end - mode->hsync_start;
		__entry->vdisplay = mode->vdisplay;
		__entry->vbp = mode->vtotal - mode->vsync_end;
		__entry->vfp = mode->vsync_start - mode->vdisplay;
		__entry->vsync = mode->vsync_end - mode->vsync_start;
	),
	TP_printk("drm_display_mode: hdisplay=%d vdisplay=%d hsync=%d vsync=%d hbp=%d hfp=%d vbp=%d vfp=%d",
		  __entry->hdisplay, __entry->vdisplay,  __entry->hsync, __entry->vsync,
		  __entry->hbp, __entry->hfp,  __entry->vbp, __entry->vfp
	)
);

TRACE_EVENT(dpu_plane_info,
	    TP_PROTO(struct drm_plane_state *state, struct drm_framebuffer *fb,
		     u32 rdma_id, u32 alpha, u32 rotation),
	TP_ARGS(state, fb, rdma_id, alpha, rotation),
	TP_STRUCT__entry(
			__field(u32, rdma_id)
			__field(u32, src_w)
			__field(u32, src_h)
			__field(u32, src_x)
			__field(u32, src_y)
			__field(u32, crtc_w)
			__field(u32, crtc_h)
			__field(u32, crtc_x)
			__field(u32, crtc_y)
			__field(u32, width)
			__field(u32, height)
			__field(u32, format)
			__field(u32, blend_mode)
			__field(u32, alpha)
			__field(u32, zpos)
			__field(u32, rotation)
	),
	TP_fast_assign(
			__entry->rdma_id = rdma_id;
			__entry->src_w = state->src_w >> 16;
			__entry->src_h = state->src_h >> 16;
			__entry->src_x = state->src_x >> 16;
			__entry->src_y = state->src_y >> 16;
			__entry->crtc_w = state->crtc_w;
			__entry->crtc_h = state->crtc_h;
			__entry->crtc_x = state->crtc_x;
			__entry->crtc_y = state->crtc_y;
			__entry->width = fb->width;
			__entry->height = fb->height;
			__entry->format = fb->format->format;
			__entry->blend_mode = state->pixel_blend_mode;
			__entry->alpha = alpha;
			__entry->zpos = state->zpos;
			__entry->rotation = rotation;
	),
	TP_printk("rdma_id=%d src: w=%d h=%d x=%d y=%d crtc: w=%d h=%d x=%d y=%d width=%d height=%d fmt=0x%x blend=%d alpha=%d zpos=%d rot=%d",
		  __entry->rdma_id, __entry->src_w,
		  __entry->src_h, __entry->src_x,
		  __entry->src_y, __entry->crtc_w,
		  __entry->crtc_h, __entry->crtc_x,
		  __entry->crtc_y, __entry->width,
		  __entry->height, __entry->format,
		  __entry->blend_mode, __entry->alpha,
		  __entry->zpos, __entry->rotation)
);

DECLARE_EVENT_CLASS(dpu_status_template,
		    TP_PROTO(const char *name, int status),
	TP_ARGS(name, status),
	TP_STRUCT__entry(
		__string(name_str, name)
		__field(int, status)
	),
	TP_fast_assign(
		__assign_str(name_str);
		__entry->status = status;
	),
	TP_printk("%s: 0x%x",
		  __get_str(name_str), __entry->status)
);

DEFINE_EVENT(dpu_status_template, dpuctrl,
	     TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, spacemit_plane_update_hw_channel,
	     TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, saturn_irq_enable,
	     TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, saturn_enable_vsync,
	     TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, dpu_isr_status,
	     TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);

DECLARE_EVENT_CLASS(dpu_uint64_t_data_template,
		    TP_PROTO(const char *name, u64 data),
	TP_ARGS(name, data),
	TP_STRUCT__entry(
		__string(name_str, name)
		__field(u64, data)
	),
	TP_fast_assign(
		__assign_str(name_str);
		__entry->data = data;
	),
	TP_printk("%s: %lld",
		  __get_str(name_str), __entry->data)
);

DEFINE_EVENT(dpu_uint64_t_data_template, u64_data,
	     TP_PROTO(const char *name, u64 data),
	TP_ARGS(name, data)
);

TRACE_EVENT(spacemit_dpu_reg_write,
	    TP_PROTO(u32 data, u32 base, const char *name),
	TP_ARGS(data, base, name),
	TP_STRUCT__entry(
		__field(u32, data)
		__field(u32, base)
		__string(name_str, name)
	),
	TP_fast_assign(
		__entry->data = data;
		__entry->base = base;
		__assign_str(name_str);
	),
	TP_printk("dpu write 0x%x to addr 0x%x field %s",
		  __entry->data, __entry->base, __get_str(name_str))
);

TRACE_EVENT(spacemit_plane_disable_hw_channel,
	    TP_PROTO(u32 layer_id, u32 rdma_id),
	TP_ARGS(layer_id, rdma_id),
	TP_STRUCT__entry(
		__field(u32, layer_id)
		__field(u32, rdma_id)
	),
	TP_fast_assign(
		__entry->layer_id = layer_id;
		__entry->rdma_id = rdma_id;
	),
	TP_printk("layer_id:%d rdma_id:%d",
		  __entry->layer_id, __entry->rdma_id)
);

DECLARE_EVENT_CLASS(dpu_func_template,
		    TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id),
	TP_STRUCT__entry(
		__field(u32, dev_id)
	),
	TP_fast_assign(
		__entry->dev_id = dev_id;
	),
	TP_printk("dev_id=%d", __entry->dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_plane_atomic_check,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_plane_atomic_update,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_plane_atomic_duplicate_state,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_plane_atomic_destroy_state,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_plane_init,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_enable_clocks,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_disable_clocks,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_uninit,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_isr,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_run,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_atomic_enable,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_atomic_disable,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_atomic_check,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_atomic_begin,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_atomic_flush,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_enable_vblank,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_disable_vblank,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_run,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_init,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, spacemit_crtc_uninit,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, saturn_ctrl_cfg_ready_timer,
	     TP_PROTO(u32 dev_id),
	TP_ARGS(dev_id)
);
DECLARE_EVENT_CLASS(dpu_ctrl_template,
		    TP_PROTO(int id, int enable),
	TP_ARGS(id, enable),
	TP_STRUCT__entry(
		__field(u32, id)
		__field(u32, enable)
	),
	TP_fast_assign(
		__entry->id = id;
		__entry->enable = enable;
	),
	TP_printk("id=%d enable:%d",
		  __entry->id, __entry->enable)
);
DEFINE_EVENT(dpu_ctrl_template, saturn_ctrl_cfg_ready,
	     TP_PROTO(int id, int enable),
	TP_ARGS(id, enable)
);

DEFINE_EVENT(dpu_ctrl_template, saturn_ctrl_sw_start,
	     TP_PROTO(int id, int enable),
	TP_ARGS(id, enable)
);

TRACE_EVENT(plat_cmdlist_dump_node,
	    TP_PROTO(char *dump),
	TP_ARGS(dump),
	TP_STRUCT__entry(
		__string(dump_str, dump)
	),
	TP_fast_assign(
		__assign_str(dump_str);
	),
	TP_printk("%s", __get_str(dump_str))
);

TRACE_EVENT(cmdlist_dump_node,
	    TP_PROTO(char *type, int rdma_id, u8 dev_id, int zpos, u16 nod_len),
	TP_ARGS(type, rdma_id, dev_id, zpos, nod_len),
	TP_STRUCT__entry(
		__string(type_str, type)
		__field(int, rdma_id)
		__field(u8, dev_id)
		__field(int, zpos)
		__field(u16, nod_len)
	),
	TP_fast_assign(
		__assign_str(type_str);
		__entry->rdma_id = rdma_id;
		__entry->dev_id = dev_id;
		__entry->zpos = zpos;
		__entry->nod_len = nod_len;
	),
	TP_printk("cmdlist[%s]: rdma_id:%d crtc:%d zpos:%d rows:%d",
		  __get_str(type_str),
		  __entry->rdma_id,
		  __entry->dev_id,
		  __entry->zpos,
		  __entry->nod_len)
);
#endif /* _DPU_TRACE_H_ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_FILE
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/gpu/drm/spacemit/dpu/

#define TRACE_INCLUDE_FILE dpu_trace
#include <trace/define_trace.h>

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef _SPACEMIT_CRTC_H_
#define _SPACEMIT_CRTC_H_

#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/regmap.h>
#include <video/videomode.h>
#include <linux/workqueue.h>

#include <drm/drm_atomic_uapi.h>
#include <drm/drm_print.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_vblank.h>
#include "spacemit_dpu_ids.h"
#include "spacemit_cmdlist.h"
#include "spacemit_drm.h"
#include <linux/spinlock.h>

#define RDMA_INVALID_ID			(~0)

#define DPU_MCLK_DEFAULT	307200000
#define DPU_AXICLK_DEFAULT	409000000

enum spacemit_dpu_irq {
	INT_UNDERRUN,
	INT_CFG_RDY,
	INT_VSYNC,
	INT_EOF,
	INT_REST,
};

struct spacemit_crtc_fbcmem {
	u32 start;
	u32 size;
	bool map;
};

struct spacemit_crtc_rdma {
	struct spacemit_crtc_fbcmem fbcmem;
	bool in_use;
	u32 use_cnt;
};

struct dpu_mmu_tbl {
	u32 size;
	void *va;
	dma_addr_t pa;
};


/**
 * struct spacemit_crtc - Per-instance state for a Saturn DPU CRTC
 * @dev: Underlying platform device for this DPU instance.
 * @crtc: Embedded DRM CRTC; reachable via to_spacemit_crtc().
 * @core: Vtable of DPU pipeline operations, populated at probe.
 * @mmu_tbl: DPU-local page table mapping planes' buffers into the TBUs.
 * @dev_id: Vendor-numbered DPU instance ID (0 or 1 on K3).
 * @irq_online: Online-compose IRQ, kept for synchronize_irq().
 * @cfg_rdy_timer: Watchdog for a CFG_RDY that never arrives.
 * @is_1st_f: True until the first frame of a new modeset is transferred.
 * @first_modeset: True until the first .atomic_enable after probe; that first
 *		   pass resets the DPU to stop the U-Boot pipeline.
 * @clk_ctx: Cached clock handles, enabled by dpu_enable_clocks().
 * @aclk: Requested DPU AXI clock rate (Hz).
 * @out_format: Output pixel format register cache.
 * @cl_rdma: Cmdlist register slot for RDMA programming.
 * @flip_done: Set once the DPU has committed the most recent page flip.
 * @clocks_on: True while clocks run; MMIO on a gated DPU stalls the bus.
 * @ur_reported: True once an underrun has been logged since the last CFG_RDY.
 * @ur_mask_armed: True while clr_int_sts() may still mask the underrun IRQ.
 */
struct spacemit_crtc {
	struct device *dev;
	struct drm_crtc crtc;
	const struct dpu_core_ops *core;
	struct dpu_mmu_tbl mmu_tbl;
	int dev_id;
	int irq_online;
	struct timer_list cfg_rdy_timer;

	bool is_1st_f;
	bool first_modeset;
	struct dpu_clk_context clk_ctx;
	u32 aclk;
	u32 out_format;

	struct cmdlist_regs *cl_rdma;

	bool flip_done;

	bool clocks_on;

	bool ur_reported;
	bool ur_mask_armed;
};

static inline struct spacemit_crtc *to_spacemit_crtc(struct drm_crtc *crtc)
{
	return crtc ? container_of(crtc, struct spacemit_crtc, crtc) : NULL;
}

struct spacemit_hw_device;
struct dpu_core_ops {
	int (*init)(struct spacemit_crtc *a_crtc);
	void (*uninit)(struct spacemit_crtc *a_crtc);
	void (*run)(struct drm_crtc *crtc,
		    struct drm_crtc_state *old_state);
	void (*disable_vsync)(struct spacemit_crtc *a_crtc);
	void (*enable_vsync)(struct spacemit_crtc *a_crtc);
	u32 (*online_isr)(struct spacemit_crtc *a_crtc);
	int (*enable_clk)(struct spacemit_crtc *a_crtc);
	int (*disable_clk)(struct spacemit_crtc *a_crtc);
	int (*cal_layer_fbcmem_size)(struct drm_plane *plane,
				     struct drm_plane_state *state);
	int (*adjust_rdma_fbcmem)(struct spacemit_hw_device *hwdev,
				  struct spacemit_crtc_rdma *rdmas);
};

extern const struct dpu_core_ops dpu_saturn_ops;

int spacemit_crtc_run(struct drm_crtc *crtc,
		      struct drm_crtc_state *old_state);

struct spacemit_plane {
	struct drm_plane plane;
	struct spacemit_hw_device *hwdev;
};

/**
 * struct spacemit_plane_state - Subclassed drm_plane_state for the Saturn DPU
 * @state: Embedded drm_plane_state; reachable via to_spacemit_plane_state().
 * @rdma_id: RDMA channel feeding this plane, derived from its zpos.
 * @format: DPU-internal pixel-format token translated from the fourcc.
 * @fbcmem_size: Bytes of FBC line buffer reserved for this plane.
 * @mmu_tbl: Per-plane page table populated by spacemit_dmmu_map().
 * @cl: Plane-level cmdlist filled during atomic_update.
 */
struct spacemit_plane_state {
	struct drm_plane_state state;
	u32 rdma_id;
	u8 format;
	u32 fbcmem_size;
	struct dpu_mmu_tbl mmu_tbl;
	struct cmdlist cl;
};

struct spacemit_plane *to_spacemit_plane(struct drm_plane *plane);

static inline struct
spacemit_plane_state *
to_spacemit_plane_state(const struct drm_plane_state *state)
{
	return container_of(state, struct spacemit_plane_state, state);
}

/**
 * struct spacemit_crtc_state - Subclassed drm_crtc_state for the Saturn DPU
 * @base: Embedded drm_crtc_state; reachable via to_spacemit_crtc_state().
 * @rdmas: Per-RDMA-channel descriptors, hwdev->rdma_nums entries.
 * @cl: CRTC-level cmdlist used during atomic_flush.
 */
struct spacemit_crtc_state {
	struct drm_crtc_state base;
	struct spacemit_crtc_rdma *rdmas;
	struct cmdlist cl;
};

#define to_spacemit_crtc_state(x) \
	container_of(x, struct spacemit_crtc_state, base)

struct drm_plane *spacemit_plane_init(struct drm_device *drm,
				      struct spacemit_crtc *a_crtc);

struct spacemit_plane_state;
struct spacemit_crtc;
struct tbu_instance;
struct dpu_mmu_tbl;
enum spacemit_dpu_irq;

/*
 * Per-revision Saturn DPU description and operation vtable. One static instance
 * per DT compatible, selected at probe; reached through spacemit_crtc::core.
 */
struct spacemit_hw_device {
	void __iomem *base;
	struct regmap *regmap;
	u8 plane_nums;
	u8 rdma_nums;
	u8 crtc_nums;
	const struct spacemit_hw_rdma *rdmas;
	u8 n_formats;
	const struct dpu_format_id *formats;
	u8 n_fbcmems;
	const u32 *fbcmem_sizes;
	void (*enable_vsync)(struct spacemit_crtc *a_crtc,
			     struct spacemit_hw_device *hwdev, bool enable);
	void (*enable_cfg_irq)(struct spacemit_crtc *a_crtc,
			       struct spacemit_hw_device *hwdev, bool enable);
	void (*cfg_ready)(struct spacemit_crtc *a_crtc,
			  struct spacemit_hw_device *hwdev);
	void (*sw_start)(struct spacemit_crtc *a_crtc,
			 struct spacemit_hw_device *hwdev);
	void (*dpu_init)(struct spacemit_crtc *a_crtc);
	void (*irq_enable)(struct spacemit_crtc *a_crtc, bool enable);
	void (*plane_update_hw_channel)(struct drm_plane *plane);
	void (*plane_disable_hw_channel)(struct drm_plane *plane,
					 struct drm_plane_state *old_state);
	void (*conf_dpuctrl)(struct drm_crtc *crtc,
			     struct drm_crtc_state *old_state);
	u32 (*get_cfg_rdy)(struct spacemit_crtc *a_crtc,
			   struct spacemit_hw_device *hwdev);
	u32 (*get_irq_bit)(enum spacemit_dpu_irq irq_id, int dev_id);
	u32 (*get_int_sts)(struct spacemit_hw_device *hwdev, int dev_id);
	void (*clr_int_sts)(struct spacemit_crtc *a_crtc, u32 data, int dev_id);
	void (*enable_cmdlist)(struct spacemit_crtc *a_crtc,
			       struct spacemit_hw_device *hwdev, int id,
			       bool enable);
	void (*cfg_cmdlist)(struct spacemit_hw_device *hwdev, int id, u32 chy,
			    u32 addrl, u32 addrh);
	void (*rdma_dmmu)(struct spacemit_hw_device *hwdev, u8 tbu_id,
			  struct tbu_instance *tbu, struct drm_framebuffer *fb,
			  u32 val, struct cmdlist_regs *cl_rdma,
			  struct drm_plane *plane);
	int (*get_cl_rdma_buf)(struct spacemit_crtc *a_crtc);
	void (*cmdlist_fill_data_row)(struct cmdlist *cl, u32 strobe,
				      u32 offset, u32 value[]);
	void (*cmdlist_fill_conf_row)(struct cmdlist *cl,
				      struct spacemit_hw_device *hwdev,
				      u8 dev_id);
	void (*crtc_cmdlist)(struct spacemit_hw_device *hwdev,
			     struct spacemit_drm_private *priv, u8 crtc_id,
			     u8 dev_id);
	void (*cmdlist_dump_node)(struct cmdlist *cl);
};
#endif

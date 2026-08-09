/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef _SPACEMIT_DMMU_H_
#define _SPACEMIT_DMMU_H_

#include <linux/types.h>

#include <drm/drm_file.h>
#include "spacemit_cmdlist.h"

#define DPU_QOS_URGENT	4
#define DPU_QOS_NORMAL	3
#define DPU_QOS_LOW	2
/* Worst-case TLB alignment over-fetch at the maximum 4096 src_x. */
#define HW_ALIGN_TTB_NUM	60

#define RD_OUTS_NUM             16
#define RDMA_TIMELIMIT  0xFFFF

#define BASE_VA         0x10000000ULL
#define VA_STEP_PER_TBU 0x40000000ULL

#define TBU_BASE_VA(tbu_id) ((u64)BASE_VA + (u64)VA_STEP_PER_TBU * (tbu_id))

struct tbu_instance {
	u64 ttb_pa[3];
	u64 tbu_va[3];
	u32 ttb_size[3];
};

struct dpu_mmu_tbl;
int spacemit_dmmu_map(struct drm_framebuffer *fb, struct dpu_mmu_tbl *mmu_tbl,
		      u8 tbu_id, struct cmdlist_regs *cl_rdma,
			struct drm_plane *plane);

#endif /* _SPACEMIT_DMMU_H_ */

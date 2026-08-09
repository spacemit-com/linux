// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023-2026 SpacemiT Co., Ltd.
 *
 */

#include <linux/stddef.h>
#include <linux/gfp.h>
#include <linux/dma-mapping.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_shmem_helper.h>

#include "spacemit_cmdlist.h"
#include "spacemit_dmmu.h"
#include "spacemit_drm.h"
#include "spacemit_crtc.h"
#include "spacemit_dpu_reg.h"

static inline void spacemit_dmmu_fill_pgtable(struct dpu_mmu_tbl *tbl,
					      struct sg_table *sgt)
{
	struct sg_dma_page_iter dma_iter;
	u32 max_entries = tbl->size / 4;
	u32 *ttbr = tbl->va;
	u32 i = 0, n = 0, temp = 0;

	for_each_sgtable_dma_page(sgt, &dma_iter, 0) {
		/*
		 * atomic_check sized the table off this same fb; catch a drift
		 */
		if (WARN_ON_ONCE(n + HW_ALIGN_TTB_NUM >= max_entries))
			break;
		ttbr[n++] = (sg_page_iter_dma_address(&dma_iter) >> PAGE_SHIFT) & 0x3FFFFFF;
	}

	if (WARN_ON_ONCE(!n))
		return;

	/*
	 * The silicon over-fetches past the last entry; pad it to avoid a DDR
	 * fault.
	 */
	temp = ttbr[n - 1];
	for (i = 0; i < HW_ALIGN_TTB_NUM; i++)
		ttbr[n++] = temp;
}

int spacemit_dmmu_map(struct drm_framebuffer *fb, struct dpu_mmu_tbl *mmu_tbl,
		      u8 tbu_id, struct cmdlist_regs *cl_rdma,
		 struct drm_plane *plane)
{
	struct spacemit_drm_private *priv = fb->dev->dev_private;
	struct spacemit_hw_device *hwdev = priv->hwdev;
	struct sg_table *sgt = NULL;
	struct tbu_instance tbu = { };
	u32 val;

	sgt = drm_gem_shmem_get_pages_sgt(to_drm_gem_shmem_obj(fb->obj[0]));
	if (IS_ERR(sgt))
		return PTR_ERR(sgt);

	/* Every advertised format is single-planar */
	tbu.ttb_pa[0] = mmu_tbl->pa;
	tbu.tbu_va[0] = TBU_BASE_VA(tbu_id);
	tbu.ttb_size[0] = roundup(fb->obj[0]->size, PAGE_SIZE) >> PAGE_SHIFT;

	spacemit_dmmu_fill_pgtable(mmu_tbl, sgt);

	/*
	 * enable = 1, fbc_mode = 0 (linear only)
	 * single plane in bits [3:2]
	 */
	val = 0x1;
	/* tbu_burst_limit_en = 1, tlb_fetch_active_en = 1 */
	val |= BIT(4) | BIT(5);
	val = val | (DPU_QOS_URGENT << 8);
	hwdev->rdma_dmmu(hwdev, tbu_id, &tbu, fb, val, cl_rdma, plane);

	return 0;
}

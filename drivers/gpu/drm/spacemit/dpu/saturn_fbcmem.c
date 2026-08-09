// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#include <linux/stddef.h>
#include <linux/export.h>
#include <drm/drm_atomic.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>
#include "../spacemit_cmdlist.h"
#include "saturn_fbcmem.h"

int get_raw_data_plane_rdma_mem_size(u32 drm_4cc_fmt, u32 plane_crop_width,
				     u32 *output_mem_size)
{
	u32 ret_mem_size = 0;
	const struct drm_format_info *info = NULL;

	info = drm_format_info(drm_4cc_fmt);
	if (info->cpp[0] == 0 || info->num_planes != 1 || info->is_yuv)
		return -EINVAL;

	/* 64-byte rounding is a dpu hardware request */
	ret_mem_size = roundup(plane_crop_width * info->cpp[0], 64);
	ret_mem_size = ret_mem_size / 32;

	if (output_mem_size)
		*output_mem_size = ret_mem_size;

	return 0;
}

int saturn_cal_layer_fbcmem_size(struct drm_plane *plane,
				 struct drm_plane_state *state)
{
	struct spacemit_plane_state *pstate = to_spacemit_plane_state(state);
	u32 drm_4cc_fmt = pstate->state.fb->format->format;
	u32 crop_w = pstate->state.src_w >> 16;

	/* Only linear sizing: the compressed path is not exposed. */
	return get_raw_data_plane_rdma_mem_size(drm_4cc_fmt, crop_w,
						&pstate->fbcmem_size);
}

/* Even RDMA (0, 2, ...): sole owner of the front of its fbc memory bank. */
static int saturn_fbcmem_place_even(struct spacemit_hw_device *hwdev,
				    struct spacemit_crtc_rdma *rdmas,
				    u32 *fbc_mems_left, u8 index)
{
	u32 cur_rdma_fbcmem_size = rdmas[index].fbcmem.size;
	u32 pri_fbcmem_size = hwdev->fbcmem_sizes[index / 2] / FBCMEM_UNIT;

	if (cur_rdma_fbcmem_size > pri_fbcmem_size)
		return -ENOSPC;

	/* The preceding odd RDMA may not already share this bank. */
	if (index > 0 && rdmas[index - 1].fbcmem.map)
		return -ENOSPC;

	rdmas[index].fbcmem.start = 0;
	rdmas[index].fbcmem.map = true;
	fbc_mems_left[index / 2] -= cur_rdma_fbcmem_size;

	return 0;
}

/* Odd RDMA takes what is left of its own bank, spilling into the next. */
static int saturn_fbcmem_place_odd(struct spacemit_hw_device *hwdev,
				   struct spacemit_crtc_rdma *rdmas,
				   u32 *fbc_mems_left, u8 index)
{
	u32 cur_rdma_fbcmem_size = rdmas[index].fbcmem.size;
	u32 pri_fbcmem_size = hwdev->fbcmem_sizes[index / 2] / FBCMEM_UNIT;
	u8 rdma_nums = hwdev->rdma_nums;
	u8 sec_fbcmem_index;
	u32 sec_fbcmem_size;

	if (cur_rdma_fbcmem_size <= fbc_mems_left[index / 2]) { /* not share fbc mem */
		rdmas[index].fbcmem.map = false;
		rdmas[index].fbcmem.start =
				pri_fbcmem_size - fbc_mems_left[index / 2];
		rdmas[index].fbcmem.size = fbc_mems_left[index / 2]; /* use all the mem left */
		fbc_mems_left[index / 2] = 0;
		return 0;
	}

	/* Need to share the next bank's fbc mem. */
	sec_fbcmem_index = (index / 2 + 1) % (rdma_nums / 2);
	sec_fbcmem_size = hwdev->fbcmem_sizes[sec_fbcmem_index] / FBCMEM_UNIT;
	if (cur_rdma_fbcmem_size > fbc_mems_left[index / 2] + sec_fbcmem_size)
		return -ENOSPC;

	if (index == rdma_nums - 1) { /* last rdma id */
		/* bank 0 is already in use */
		if (fbc_mems_left[0] != hwdev->fbcmem_sizes[0] / FBCMEM_UNIT)
			return -ENOSPC;

		rdmas[index].fbcmem.size =
				fbc_mems_left[index / 2] + sec_fbcmem_size;
		cur_rdma_fbcmem_size = rdmas[index].fbcmem.size;
	}
	fbc_mems_left[sec_fbcmem_index] -= (cur_rdma_fbcmem_size - fbc_mems_left[index / 2]);
	rdmas[index].fbcmem.start = pri_fbcmem_size - fbc_mems_left[index / 2];
	rdmas[index].fbcmem.map = true;
	fbc_mems_left[index / 2] = 0;

	return 0;
}

int saturn_adjust_rdma_fbcmem(struct spacemit_hw_device *hwdev,
			      struct spacemit_crtc_rdma *rdmas)
{
	int ret = -ENOSPC;
	u8 index = 0;
	u32 *fbc_mems_left = NULL;
	u8 rdma_nums = hwdev->rdma_nums;

	fbc_mems_left = kzalloc_objs(*fbc_mems_left, rdma_nums / 2);
	if (!fbc_mems_left) {
		ret = -ENOMEM;
		goto free;
	}
	for (index = 0; index < rdma_nums / 2; index++)
		fbc_mems_left[index] = hwdev->fbcmem_sizes[index] / FBCMEM_UNIT;

	for (index = 0; index < rdma_nums; index++) {
		int err;

		if (rdmas[index].fbcmem.size == 0)
			continue;

		if (index % 2 == 0)
			err = saturn_fbcmem_place_even(hwdev, rdmas,
						       fbc_mems_left, index);
		else
			err = saturn_fbcmem_place_odd(hwdev, rdmas,
						      fbc_mems_left, index);
		if (err) {
			ret = err;
			goto free;
		}
	}
	ret = 0;

free:
	kfree(fbc_mems_left);

	return ret;
}

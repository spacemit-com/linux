/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SATURN_FBCMEM_H_
#define _SATURN_FBCMEM_H_

#include <drm/drm_plane.h>
#include "../spacemit_crtc.h"
#include "../spacemit_dpu_reg.h"
#include "dpu_saturn.h"

#define FBCMEM_UNIT	(32) /* fbcmem is 32 bytes per unit */

int get_raw_data_plane_rdma_mem_size(u32 drm_4cc_fmt, u32 plane_crop_width,
				     u32 *output_mem_size);

int saturn_cal_layer_fbcmem_size(struct drm_plane *plane,
				 struct drm_plane_state *state);

int saturn_adjust_rdma_fbcmem(struct spacemit_hw_device *hwdev,
			      struct spacemit_crtc_rdma *rdmas);

#endif

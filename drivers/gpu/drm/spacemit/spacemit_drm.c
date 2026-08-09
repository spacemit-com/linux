// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_fbdev_shmem.h>
#include <linux/component.h>
#include <linux/of_graph.h>

#include "spacemit_drm.h"
#include "spacemit_crtc.h"

#define DRIVER_NAME	"spacemit"
#define DRIVER_DESC	"SpacemiT SoC DRM driver"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

static void spacemit_drm_atomic_commit_tail(struct drm_atomic_commit *old_state)
{
	struct drm_device *dev = old_state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, old_state);

	drm_atomic_helper_commit_modeset_enables(dev, old_state);

	drm_atomic_helper_commit_planes(dev, old_state,
					DRM_PLANE_COMMIT_ACTIVE_ONLY);

	/*
	 * Flip first, hw_done second: the cmdlist engine reads the old
	 * state's DMA buffers until the frame boundary, and hw_done both
	 * releases the next commit to start programming and lets the old
	 * state (with those buffers) be torn down.
	 */
	drm_atomic_helper_wait_for_flip_done(dev, old_state);

	drm_atomic_helper_commit_hw_done(old_state);

	drm_atomic_helper_cleanup_planes(dev, old_state);
}

static const struct drm_mode_config_helper_funcs spacemit_drm_mode_config_helper = {
	.atomic_commit_tail = spacemit_drm_atomic_commit_tail,
};

static const struct drm_mode_config_funcs spacemit_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

int spacemit_drm_mode_config_init(struct drm_device *drm)
{
	int ret;

	ret = drmm_mode_config_init(drm);
	if (ret)
		return ret;

	drm->mode_config.min_width = 1;
	drm->mode_config.min_height = 1;
	/* K3 user manual: each DPU scans out at most 3840x2160@60 */
	drm->mode_config.max_width = 3840;
	drm->mode_config.max_height = 2160;

	drm->mode_config.funcs = &spacemit_drm_mode_config_funcs;
	drm->mode_config.helper_private = &spacemit_drm_mode_config_helper;

	return 0;
}

DEFINE_DRM_GEM_FOPS(spacemit_drm_fops);

struct drm_driver spacemit_drm_drv = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &spacemit_drm_fops,

	DRM_GEM_SHMEM_DRIVER_OPS,
	DRM_FBDEV_SHMEM_DRIVER_OPS,

	.name		= DRIVER_NAME,
	.desc		= DRIVER_DESC,
	.major		= DRIVER_MAJOR,
	.minor		= DRIVER_MINOR,
};

static int compare_of(struct device *dev, void *data)
{
	return dev->of_node == data;
}

int spacemit_drm_of_component_probe(struct device *dev)
{
	struct device_node *ports, *port, *ep;
	struct device_node *remote;
	struct component_match *match = NULL;

	if (!dev->of_node)
		return -EINVAL;

	/* The DPU is the drm root: its OF graph names the DP/eDP components. */
	ports = of_get_child_by_name(dev->of_node, "ports");
	if (!ports) {
		dev_err(dev, "missing 'ports' child node\n");
		return -EINVAL;
	}

	for_each_child_of_node(ports, port) {
		if (!of_node_name_eq(port, "port"))
			continue;
		for_each_child_of_node(port, ep) {
			if (!of_node_name_eq(ep, "endpoint"))
				continue;
			remote = of_graph_get_remote_port_parent(ep);
			if (!remote)
				continue;
			if (of_device_is_available(remote))
				drm_of_component_match_add(dev, &match,
							   compare_of, remote);
			of_node_put(remote);
		}
	}
	of_node_put(ports);

	if (!match) {
		dev_err(dev, "no downstream component available\n");
		return -ENODEV;
	}
	if (IS_ERR(match))
		return PTR_ERR(match);

	return component_master_add_with_match(dev, &spacemit_drm_master_ops,
					       match);
}

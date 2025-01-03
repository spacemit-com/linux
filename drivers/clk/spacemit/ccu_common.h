/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#ifndef _CCU_COMMON_H_
#define _CCU_COMMON_H_

#include <linux/regmap.h>

struct ccu_common {
	struct regmap *base;
	struct regmap *lock_base;

	union {
		struct {
			u32 reg_ctrl;
			u32 reg_sel;
			u32 reg_fc;
			u32 fc;
		};
		struct {
			u32 reg_swcr1;
			u32 reg_swcr2;
			u32 reg_swcr3;
		};
	};

	unsigned long flags;
	const char *name;
	const char * const *parent_names;
	int num_parents;

	struct clk_hw hw;
};

static inline struct ccu_common *hw_to_ccu_common(struct clk_hw *hw)
{
	return container_of(hw, struct ccu_common, hw);
}

#define ccu_read(reg, c, val)	regmap_read((c)->base, (c)->reg_##reg, val)
#define ccu_write(reg, c, val)	regmap_write((c)->base, (c)->reg_##reg, val)
#define ccu_update(reg, c, mask, val) \
	regmap_update_bits((c)->base, (c)->reg_##reg, mask, val)
#define ccu_poll(reg, c, tmp, cond, sleep, timeout) \
	regmap_read_poll_timeout_atomic((c)->base, (c)->reg_##reg,	\
					tmp, cond, sleep, timeout)

#endif /* _CCU_COMMON_H_ */

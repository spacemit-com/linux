/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#ifndef _CCU_PLL_H_
#define _CCU_PLL_H_

#include <linux/clk-provider.h>

#include "ccu_common.h"

struct ccu_pll_rate_tbl {
	unsigned long long rate;
	u32 reg5;
	u32 reg6;
	u32 reg7;
	u32 reg8;
	unsigned int div_int;
	unsigned int div_frac;
};

struct ccu_pll_config {
	struct ccu_pll_rate_tbl *rate_tbl;
	u32 tbl_size;
	u32 reg_lock;
	u32 lock_enable_bit;
};

#define CCU_PLL_RATE(_rate, _reg5, _reg6, _reg7, _reg8, _div_int, _div_frac) \
	{									\
		.rate		= (_rate),					\
		.reg5		= (_reg5),					\
		.reg6		= (_reg6),					\
		.reg7		= (_reg7),					\
		.reg8		= (_reg8),					\
		.div_int	= (_div_int),				\
		.div_frac	= (_div_frac),				\
	}

struct ccu_pll {
	struct ccu_pll_config	pll;
	struct ccu_common	common;
};

#define CCU_PLL_CONFIG(_table, _reg_lock, _lock_enable_bit) \
	{									\
		.rate_tbl	 = (struct ccu_pll_rate_tbl *)&(_table),	\
		.tbl_size	 = ARRAY_SIZE(_table),				\
		.reg_lock	 = (_reg_lock),					\
		.lock_enable_bit = (_lock_enable_bit),				\
	}

#define CCU_PLL_HWINIT(_name, _flags) \
	CLK_HW_INIT_NO_PARENT(_name, &spacemit_ccu_pll_ops, _flags)

#define CCU_PLL_DEFINE(_struct, _name, _table, _reg_swcr1, _reg_swcr2,	\
		       _reg_swcr3, _reg_lock, _lock_enable_bit, _flags)		\
										\
	struct ccu_pll _struct = {						\
		.pll	= CCU_PLL_CONFIG(_table, _reg_lock, _lock_enable_bit),	\
		.common = {							\
			.reg_swcr1	= _reg_swcr1,				\
			.reg_swcr2	= _reg_swcr2,				\
			.reg_swcr3	= _reg_swcr3,				\
			.hw.init	= CCU_PLL_HWINIT(_name, _flags)		\
		}								\
	}

static inline struct ccu_pll *hw_to_ccu_pll(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_pll, common);
}

extern const struct clk_ops spacemit_ccu_pll_ops;

#endif

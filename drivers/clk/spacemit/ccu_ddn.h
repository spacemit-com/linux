/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#ifndef _CCU_DDN_H_
#define _CCU_DDN_H_

#include <linux/clk-provider.h>

#include "ccu_common.h"

struct ccu_ddn_tbl {
	unsigned int num;
	unsigned int den;
};

struct ccu_ddn_info {
	unsigned int factor;
	unsigned int num_mask;
	unsigned int den_mask;
	unsigned int num_shift;
	unsigned int den_shift;
};

struct ccu_ddn_config {
	struct ccu_ddn_info *info;
	struct ccu_ddn_tbl *tbl;
	u32 tbl_size;
};

struct ccu_ddn {
	struct ccu_ddn_config  ddn;
	struct ccu_common	common;
	u32 gate;
};

#define CCU_DDN_CONFIG(_info, _table)					\
	{								\
		.info		= (struct ccu_ddn_info *)_info,		\
		.tbl		= (struct ccu_ddn_tbl *)&_table,	\
		.tbl_size	= ARRAY_SIZE(_table),			\
	}

#define CCU_DDN_INIT(_name, _parent, _ops, _flags) \
	CLK_HW_INIT_HW(_name, &_parent.common.hw, &_ops, _flags)

#define CCU_DDN_DEFINE(_struct, _name, _parent, _info, _table,			\
		       _reg_ctrl, _flags)					\
	struct ccu_ddn _struct = {						\
		.ddn	= CCU_DDN_CONFIG(_info, _table),			\
		.common = {							\
			.reg_ctrl = _reg_ctrl,					\
			.hw.init  = CCU_DDN_INIT(_name, _parent,		\
						 spacemit_ccu_ddn_ops,		\
						 _flags),			\
		}								\
	}

#define CCU_DDN_GATE_DEFINE(_struct, _name, _parent, _info, _table,		\
			    _reg_ddn, _reg_gate, _gate_mask, _flags)		\
	struct ccu_ddn _struct = {						\
		.ddn	= CCU_DDN_CONFIG(_info, _table),			\
		.common = {							\
			.reg_ctrl	= _reg_ddn,				\
			.reg_sel	= _reg_gate,				\
			.hw.init = CCU_DDN_INIT(_name, _parent,			\
						&spacemit_ccu_ddn_gate_ops,	\
						_flags),			\
		}								\
		.gate	= _gate_mask,						\
	}

static inline struct ccu_ddn *hw_to_ccu_ddn(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_ddn, common);
}

extern const struct clk_ops spacemit_ccu_ddn_ops, spacemit_ccu_ddn_gate_ops;

#endif

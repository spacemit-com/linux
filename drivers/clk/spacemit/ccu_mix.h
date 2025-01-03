/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#ifndef _CCU_MIX_H_
#define _CCU_MIX_H_

#include <linux/clk-provider.h>

#include "ccu_common.h"

struct ccu_gate_config {
	u32 gate_mask;
	u32 val_enable;
	u32 val_disable;
	u32 flags;
};

struct ccu_factor_config {
	u32 div;
	u32 mul;
};

struct ccu_mux_config {
	const u8 *table;
	u32 flags;
	u8 shift;
	u8 width;
};

struct ccu_div_config {
	struct clk_div_table *table;
	u32 max;
	u32 offset;
	u32 flags;
	u8 shift;
	u8 width;
};

struct ccu_mix {
	struct ccu_factor_config *factor;
	struct ccu_gate_config *gate;
	struct ccu_div_config *div;
	struct ccu_mux_config *mux;
	struct ccu_common common;
};

#define CCU_GATE_INIT(_gate_mask, _val_enable, _val_disable, _flags)	\
	(&(struct ccu_gate_config) {					\
		.gate_mask   = _gate_mask,				\
		.val_enable  = _val_enable,				\
		.val_disable = _val_disable,				\
		.flags	     = _flags,					\
	})

#define CCU_FACTOR_INIT(_div, _mul)					\
	(&(struct ccu_factor_config) {					\
		.div = _div,						\
		.mul = _mul,						\
	})


#define CCU_MUX_INIT(_shift, _width, _table, _flags)			\
	(&(struct ccu_mux_config) {					\
		.shift	= _shift,					\
		.width	= _width,					\
		.table	= _table,					\
		.flags	= _flags,					\
	})

#define CCU_DIV_INIT(_shift, _width, _table, _flags)			\
	(&(struct ccu_div_config) {					\
		.shift	= _shift,					\
		.width	= _width,					\
		.flags	= _flags,					\
		.table	= _table,					\
	})

#define CCU_PARENT_HW(_parent)		{ .hw = &_parent.common.hw }
#define CCU_PARENT_NAME(_name)		{ .fw_name = #_name }

#define CCU_MIX_INITHW(_name, _parent, _ops, _flags)			\
	(&(struct clk_init_data) {					\
		.flags		= _flags,				\
		.name		= _name,				\
		.parent_data	= (const struct clk_parent_data[])	\
					{ _parent },			\
		.num_parents	= 1,					\
		.ops		= &_ops,				\
	})

#define CCU_MIX_INITHW_PARENTS(_name, _parents, _ops, _flags)		\
	CLK_HW_INIT_PARENTS_DATA(_name, _parents,			\
				 &_ops, _flags)

#define CCU_GATE_DEFINE(_struct, _name, _parent, _reg, _gate_mask,		\
			 _val_enable, _val_disable, _flags)			\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.common	= {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= 1,						\
		.hw.init = CCU_MIX_INITHW(_name, _parent,			\
					  spacemit_ccu_gate_ops, _flags),	\
	}									\
}

#define CCU_FACTOR_DEFINE(_struct, _name, _parent, _div, _mul)			\
struct ccu_mix _struct = {							\
	.factor	= CCU_FACTOR_INIT(_div, _mul),					\
	.common = {								\
		.name		= _name,					\
		.num_parents	= 1,						\
		.hw.init = CCU_MIX_INITHW(_name, _parent,			\
					  spacemit_ccu_factor_ops, 0),		\
	}									\
}

#define CCU_MUX_DEFINE(_struct, _name, _parents, _reg, _shift, _width,		\
		       _flags)							\
struct ccu_mix _struct = {							\
	.mux	= CCU_MUX_INIT(_shift, _width, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_mux_ops,	_flags),\
	}									\
}

#define CCU_DIV_DEFINE(_struct, _name, _parent, _reg, _shift, _width,		\
		       _flags)							\
struct ccu_mix _struct = {							\
	.div	= CCU_DIV_INIT(_shift, _width, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= 1,						\
		.hw.init = CCU_MIX_INITHW(_name, _parent,			\
					  spacemit_ccu_div_ops, _flags)		\
	}									\
}

#define CCU_GATE_FACTOR_DEFINE(_struct, _name, _parent, _reg,			\
			       _gate_mask, _val_enable, _val_disable,		\
			       _div, _mul, _flags)				\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.factor	= CCU_FACTOR_INIT(_div, _mul),					\
	.common = {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= 1,						\
		.hw.init = CCU_MIX_INITHW(_name, _parent,			\
					  spacemit_ccu_gate_factor_ops, _flags)	\
	}									\
}

#define CCU_MUX_GATE_DEFINE(_struct, _name, _parents, _reg, _shift,		\
			    _width, _gate_mask, _val_enable,			\
			    _val_disable, _flags)				\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.mux	= CCU_MUX_INIT(_shift, _width, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_mux_gate_ops,	\
						  _flags),			\
	}									\
}

#define CCU_DIV_GATE_DEFINE(_struct, _name, _parent, _reg, _shift,		\
			    _width, _gate_mask, _val_enable,			\
			    _val_disable, _flags)				\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.div	= CCU_DIV_INIT(_shift, _width, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg,						\
		.name		= _name,					\
		.num_parents	= 1,						\
		.hw.init = CCU_MIX_INITHW(_name, _parent,			\
					  spacemit_ccu_div_gate_ops, _flags),	\
	}									\
}

#define CCU_DIV_MUX_GATE_DEFINE(_struct, _name, _parents,  _reg_ctrl,		\
				_mshift, _mwidth, _muxshift, _muxwidth,		\
				_gate_mask, _val_enable, _val_disable,		\
				_flags)						\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),			\
	.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0),			\
	.common	= {								\
		.reg_ctrl	= _reg_ctrl,					\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_div_mux_gate_ops,\
						  _flags),			\
	},									\
}

#define CCU_DIV2_FC_MUX_GATE_DEFINE(_struct, _name, _parents,			\
				    _reg_ctrl, _reg_fc, _mshift,		\
				    _mwidth, _fc, _muxshift, _muxwidth,		\
				    _gate_mask, _val_enable,			\
				    _val_disable, _flags)			\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),			\
	.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg_ctrl,					\
		.reg_fc		= _reg_fc,					\
		.fc		= _fc,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_div_mux_gate_ops,\
						  _flags),			\
	},									\
}

#define CCU_DIV_FC_MUX_GATE_DEFINE(_struct, _name, _parents, _reg_ctrl,		\
				   _mshift, _mwidth, _fc, _muxshift,		\
				   _muxwidth, _gate_mask, _val_enable,		\
				   _val_disable, _flags)			\
struct ccu_mix _struct = {							\
	.gate	= CCU_GATE_INIT(_gate_mask, _val_enable,			\
				_val_disable, 0),				\
	.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),			\
	.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg_ctrl,					\
		.reg_fc		= _reg_ctrl,					\
		.fc		= _fc,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_div_mux_gate_ops,\
						  _flags),			\
	},									\
}

#define CCU_DIV_FC_MUX_DEFINE(_struct, _name, _parents, _reg_ctrl,		\
			      _mshift, _mwidth, _fc, _muxshift,			\
			      _muxwidth, _flags)				\
struct ccu_mix _struct = {							\
	.div	= CCU_DIV_INIT(_mshift, _mwidth, NULL, 0),			\
	.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg_ctrl,					\
		.reg_fc		= _reg_ctrl,					\
		.fc		= _fc,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_div_mux_ops,	\
						  _flags),			\
	},									\
}

#define CCU_MUX_FC_DEFINE(_struct, _name, _parents, _reg_ctrl, _fc,		\
			  _muxshift, _muxwidth, _flags)				\
struct ccu_mix _struct = {							\
	.mux	= CCU_MUX_INIT(_muxshift, _muxwidth, NULL, 0),			\
	.common = {								\
		.reg_ctrl	= _reg_ctrl,					\
		.reg_fc		= _reg_ctrl,					\
		.fc		= _fc,						\
		.name		= _name,					\
		.num_parents	= ARRAY_SIZE(_parents),				\
		.hw.init = CCU_MIX_INITHW_PARENTS(_name, _parents,		\
						  spacemit_ccu_mux_ops,	_flags)	\
	},									\
}

static inline struct ccu_mix *hw_to_ccu_mix(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_mix, common);
}

extern const struct clk_ops spacemit_ccu_gate_ops, spacemit_ccu_factor_ops;
extern const struct clk_ops spacemit_ccu_mux_ops, spacemit_ccu_div_ops;

extern const struct clk_ops spacemit_ccu_gate_factor_ops;
extern const struct clk_ops spacemit_ccu_div_gate_ops;
extern const struct clk_ops spacemit_ccu_mux_gate_ops;
extern const struct clk_ops spacemit_ccu_div_mux_ops;

extern const struct clk_ops spacemit_ccu_div_mux_gate_ops;
#endif /* _CCU_DIV_H_ */

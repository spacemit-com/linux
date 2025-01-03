// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type mix(div/mux/gate/factor)
 *
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#include <linux/clk-provider.h>

#include "ccu_mix.h"

#define MIX_TIMEOUT	10000

static void ccu_gate_disable(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;

	ccu_update(ctrl, common, gate->gate_mask, gate->val_disable);
}

static int ccu_gate_enable(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 val_enable, mask;
	u32 tmp;

	val_enable	= gate->val_enable;
	mask		= gate->gate_mask;

	ccu_update(ctrl, common, mask, val_enable);

	return ccu_poll(ctrl, common, tmp, (tmp & mask) == val_enable,
			10, MIX_TIMEOUT);
}

static int ccu_gate_is_enabled(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_gate_config *gate = mix->gate;
	u32 tmp;

	ccu_read(ctrl, common, &tmp);

	return (tmp & gate->gate_mask) == gate->val_enable;
}

static unsigned long ccu_factor_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);

	return parent_rate * mix->factor->mul / mix->factor->div;
}

static unsigned long ccu_div_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_div_config *div = mix->div;
	unsigned long val;
	u32 reg;

	ccu_read(ctrl, common, &reg);

	val = reg >> div->shift;
	val &= (1 << div->width) - 1;

	val = divider_recalc_rate(hw, parent_rate, val, div->table,
				  div->flags, div->width);

	return val;
}

static int ccu_mix_trigger_fc(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	unsigned int val = 0;

	ccu_update(fc, common, common->fc, common->fc);

	return ccu_poll(fc, common, val, !(val & common->fc),
			5, MIX_TIMEOUT);
}

static long ccu_factor_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *prate)
{
	return ccu_factor_recalc_rate(hw, *prate);
}

static int ccu_factor_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	return 0;
}

static unsigned long
ccu_mix_calc_best_rate(struct clk_hw *hw, unsigned long rate,
		       struct clk_hw **best_parent,
		       unsigned long *best_parent_rate,
		       u32 *div_val)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_div_config *div = mix->div;
	u32 div_max = div ? 1 << div->width : 1;
	unsigned long best_rate = 0;

	for (int i = 0; i < common->num_parents; i++) {
		struct clk_hw *parent = clk_hw_get_parent_by_index(hw, i);

		if (!parent)
			continue;

		unsigned long parent_rate = clk_hw_get_rate(parent);

		for (int j = 1; j <= div_max; j++) {
			unsigned long tmp = DIV_ROUND_UP_ULL(parent_rate, j);

			if (abs(tmp - rate) < abs(best_rate - rate)) {
				best_rate = tmp;

				if (div_val)
					*div_val = j - 1;

				if (best_parent) {
					*best_parent      = parent;
					*best_parent_rate = parent_rate;
				}
			}
		}
	}

	return best_rate;
}

static int ccu_mix_determine_rate(struct clk_hw *hw,
				  struct clk_rate_request *req)
{
	req->rate = ccu_mix_calc_best_rate(hw, req->rate,
					   &req->best_parent_hw,
					   &req->best_parent_rate,
					   NULL);
	return 0;
}

static int ccu_mix_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_div_config *div = mix->div;
	int ret = 0, tmp = 0;
	u32 current_div, target_div;

	ccu_mix_calc_best_rate(hw, rate, NULL, NULL, &target_div);

	ccu_read(ctrl, common, &tmp);

	current_div = tmp >> div->shift;
	current_div &= (1 << div->width) - 1;

	if (current_div == target_div)
		return 0;

	tmp = GENMASK(div->width + div->shift - 1, div->shift);

	ccu_update(ctrl, common, tmp, target_div << div->shift);

	if (common->reg_fc)
		ret = ccu_mix_trigger_fc(hw);

	return ret;
}

static u8 ccu_mux_get_parent(struct clk_hw *hw)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	u32 reg;
	u8 parent;

	ccu_read(ctrl, common, &reg);

	parent = reg >> mux->shift;
	parent &= (1 << mux->width) - 1;

	if (mux->table) {
		int num_parents = clk_hw_get_num_parents(&common->hw);
		int i;

		for (i = 0; i < num_parents; i++)
			if (mux->table[i] == parent)
				return i;
	}

	return parent;
}

static int ccu_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct ccu_mix *mix = hw_to_ccu_mix(hw);
	struct ccu_common *common = &mix->common;
	struct ccu_mux_config *mux = mix->mux;
	int ret = 0;
	u32 mask;

	if (mux->table)
		index = mux->table[index];

	mask = GENMASK(mux->width + mux->shift - 1, mux->shift);

	ccu_update(ctrl, common, mask, index << mux->shift);

	if (common->reg_fc)
		ret = ccu_mix_trigger_fc(hw);

	return ret;
}

const struct clk_ops spacemit_ccu_gate_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,
};

const struct clk_ops spacemit_ccu_factor_ops = {
	.round_rate	= ccu_factor_round_rate,
	.recalc_rate	= ccu_factor_recalc_rate,
	.set_rate	= ccu_factor_set_rate,
};

const struct clk_ops spacemit_ccu_mux_ops = {
	.determine_rate = ccu_mix_determine_rate,
	.get_parent	= ccu_mux_get_parent,
	.set_parent	= ccu_mux_set_parent,
};

const struct clk_ops spacemit_ccu_div_ops = {
	.determine_rate = ccu_mix_determine_rate,
	.recalc_rate	= ccu_div_recalc_rate,
	.set_rate	= ccu_mix_set_rate,
};

const struct clk_ops spacemit_ccu_gate_factor_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,

	.round_rate	= ccu_factor_round_rate,
	.recalc_rate	= ccu_factor_recalc_rate,
	.set_rate	= ccu_factor_set_rate,
};

const struct clk_ops spacemit_ccu_mux_gate_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,

	.determine_rate = ccu_mix_determine_rate,
	.get_parent	= ccu_mux_get_parent,
	.set_parent	= ccu_mux_set_parent,
};

const struct clk_ops spacemit_ccu_div_gate_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,

	.determine_rate = ccu_mix_determine_rate,
	.recalc_rate	= ccu_div_recalc_rate,
	.set_rate	= ccu_mix_set_rate,
};

const struct clk_ops spacemit_ccu_div_mux_gate_ops = {
	.disable	= ccu_gate_disable,
	.enable		= ccu_gate_enable,
	.is_enabled	= ccu_gate_is_enabled,

	.get_parent	= ccu_mux_get_parent,
	.set_parent	= ccu_mux_set_parent,

	.determine_rate = ccu_mix_determine_rate,
	.recalc_rate	= ccu_div_recalc_rate,
	.set_rate	= ccu_mix_set_rate,
};

const struct clk_ops spacemit_ccu_div_mux_ops = {
	.get_parent	= ccu_mux_get_parent,
	.set_parent	= ccu_mux_set_parent,

	.determine_rate = ccu_mix_determine_rate,
	.recalc_rate	= ccu_div_recalc_rate,
	.set_rate	= ccu_mix_set_rate,
};

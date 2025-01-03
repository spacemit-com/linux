// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type ddn
 *
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#include <linux/clk-provider.h>

#include "ccu_ddn.h"

/*
 * It is M/N clock
 *
 * Fout from synthesizer can be given from two equations:
 * numerator/denominator = Fin / (Fout * factor)
 */
static void ccu_ddn_disable(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common *common = &ddn->common;

	ccu_update(sel, common, ddn->gate, 0);
}

static int ccu_ddn_enable(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common *common = &ddn->common;

	ccu_update(sel, common, ddn->gate, ddn->gate);

	return 0;
}

static int ccu_ddn_is_enabled(struct clk_hw *hw)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_common *common = &ddn->common;
	u32 tmp;

	ccu_read(sel, common, &tmp);

	return tmp & ddn->gate;
}

static long clk_ddn_round_rate(struct clk_hw *hw, unsigned long drate,
			       unsigned long *prate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned long rate = 0, prev_rate;
	unsigned long result;
	int i;

	for (i = 0; i < params->tbl_size; i++) {
		prev_rate = rate;
		rate = (*prate * params->tbl[i].den) /
			(params->tbl[i].num * params->info->factor);
		if (rate > drate)
			break;
	}

	if ((i == 0) || (i == params->tbl_size)) {
		result = rate;
	} else {
		if ((drate - prev_rate) > (rate - drate))
			result = rate;
		else
			result = prev_rate;
	}

	return result;
}

static unsigned long clk_ddn_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	unsigned int val, num, den;
	unsigned long rate;

	ccu_read(ctrl, &ddn->common, &val);

	num = (val >> params->info->num_shift) & params->info->num_mask;
	den = (val >> params->info->den_shift) & params->info->den_mask;

	if (!den)
		return 0;

	rate = (parent_rate * den) / (num * params->info->factor);

	return rate;
}

/* Configures new clock rate*/
static int clk_ddn_set_rate(struct clk_hw *hw, unsigned long drate,
			    unsigned long prate)
{
	struct ccu_ddn *ddn = hw_to_ccu_ddn(hw);
	struct ccu_ddn_config *params = &ddn->ddn;
	struct ccu_ddn_info *info = params->info;
	unsigned long rate = 0;
	int i;

	for (i = 0; i < params->tbl_size; i++) {
		rate = (prate * params->tbl[i].den) /
		       (params->tbl[i].num * info->factor);

		if (rate > drate)
			break;
	}

	if (i > 0)
		i--;

	ccu_update(ctrl, &ddn->common,
		   info->num_mask | info->den_mask,
		   (params->tbl[i].num << info->num_shift) |
		   (params->tbl[i].den << info->den_shift));

	return 0;
}

const struct clk_ops spacemit_ccu_ddn_ops = {
	.recalc_rate	= clk_ddn_recalc_rate,
	.round_rate	= clk_ddn_round_rate,
	.set_rate	= clk_ddn_set_rate,
};

const struct clk_ops spacemit_ccu_ddn_gate_ops = {
	.disable	= ccu_ddn_disable,
	.enable		= ccu_ddn_enable,
	.is_enabled	= ccu_ddn_is_enabled,
	.recalc_rate	= clk_ddn_recalc_rate,
	.round_rate	= clk_ddn_round_rate,
	.set_rate	= clk_ddn_set_rate,
};

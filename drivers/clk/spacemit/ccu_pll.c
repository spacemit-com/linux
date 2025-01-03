// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit clock type pll
 *
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#include <linux/clk-provider.h>
#include <linux/regmap.h>

#include "ccu_common.h"
#include "ccu_pll.h"

#define PLL_MIN_FREQ	600000000
#define PLL_MAX_FREQ	3400000000
#define PLL_DELAY_TIME	3000

#define PLL_SWCR1_REG5_OFF	0
#define PLL_SWCR1_REG5_MASK	GENMASK(7, 0)
#define PLL_SWCR1_REG6_OFF	8
#define PLL_SWCR1_REG6_MASK	GENMASK(15, 8)
#define PLL_SWCR1_REG7_OFF	16
#define PLL_SWCR1_REG7_MASK	GENMASK(23, 16)
#define PLL_SWCR1_REG8_OFF	24
#define PLL_SWCR1_REG8_MASK	GENMASK(31, 24)

#define PLL_SWCR2_DIVn_EN(n)	BIT(n + 1)
#define PLL_SWCR2_ATEST_EN	BIT(12)
#define PLL_SWCR2_CKTEST_EN	BIT(13)
#define PLL_SWCR2_DTEST_EN	BIT(14)

#define PLL_SWCR3_DIV_FRC_OFF	0
#define PLL_SWCR3_DIV_FRC_MASK	GENMASK(23, 0)
#define PLL_SWCR3_DIV_INT_OFF	24
#define PLL_SWCR3_DIV_INT_MASK	GENMASK(30, 24)
#define PLL_SWCR3_EN		BIT(31)

static int ccu_pll_is_enabled(struct clk_hw *hw)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	u32 tmp;

	ccu_read(swcr3, &p->common, &tmp);

	return tmp & PLL_SWCR3_EN;
}

/* frequency unit Mhz, return pll vco freq */
static unsigned long ccu_pll_get_vco_freq(struct clk_hw *hw)
{
	unsigned int reg5, reg6, reg7, reg8, size, i;
	unsigned int div_int, div_frc;
	struct ccu_pll_rate_tbl *freq_pll_regs_table;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_common *common = &p->common;
	u32 tmp;

	ccu_read(swcr1, common, &tmp);
	reg5 = (tmp & PLL_SWCR1_REG5_MASK) >> PLL_SWCR1_REG5_OFF;
	reg6 = (tmp & PLL_SWCR1_REG6_MASK) >> PLL_SWCR1_REG6_OFF;
	reg7 = (tmp & PLL_SWCR1_REG7_MASK) >> PLL_SWCR1_REG7_OFF;
	reg8 = (tmp & PLL_SWCR1_REG8_MASK) >> PLL_SWCR1_REG8_OFF;

	ccu_read(swcr3, common, &tmp);
	div_int = (tmp & PLL_SWCR3_DIV_INT_MASK) >> PLL_SWCR3_DIV_INT_OFF;
	div_frc = (tmp & PLL_SWCR3_DIV_FRC_MASK) >> PLL_SWCR3_DIV_FRC_OFF;

	freq_pll_regs_table = p->pll.rate_tbl;
	size = p->pll.tbl_size;

	for (i = 0; i < size; i++)
		if ((freq_pll_regs_table[i].reg5 == reg5) &&
		    (freq_pll_regs_table[i].reg6 == reg6) &&
		    (freq_pll_regs_table[i].reg7 == reg7) &&
		    (freq_pll_regs_table[i].reg8 == reg8) &&
		    (freq_pll_regs_table[i].div_int == div_int) &&
		    (freq_pll_regs_table[i].div_frac == div_frc))
			return freq_pll_regs_table[i].rate;

	WARN_ON_ONCE(1);

	return 0;
}

static int ccu_pll_enable(struct clk_hw *hw)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_common *common = &p->common;
	unsigned int tmp;
	int ret;

	if (ccu_pll_is_enabled(hw))
		return 0;

	ccu_update(swcr3, common, PLL_SWCR3_EN, PLL_SWCR3_EN);

	/* check lock status */
	ret = regmap_read_poll_timeout_atomic(common->lock_base,
					      p->pll.reg_lock,
					      tmp,
					      tmp & p->pll.lock_enable_bit,
					      5, PLL_DELAY_TIME);

	return ret;
}

static void ccu_pll_disable(struct clk_hw *hw)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_common *common = &p->common;

	ccu_update(swcr3, common, PLL_SWCR3_EN, 0);
}

/*
 * pll rate change requires sequence:
 * clock off -> change rate setting -> clock on
 * This function doesn't really change rate, but cache the config
 */
static int ccu_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_common *common = &p->common;
	struct ccu_pll_config *params = &p->pll;
	struct ccu_pll_rate_tbl *entry = NULL;
	u32 mask, val;
	int i;

	for (i = 0; i < params->tbl_size; i++) {
		if (rate == params->rate_tbl[i].rate) {
			entry = &params->rate_tbl[i];
			break;
		}
	}

	if (WARN_ON_ONCE(!entry))
		return -EINVAL;

	mask = PLL_SWCR1_REG5_MASK | PLL_SWCR1_REG6_MASK;
	mask |= PLL_SWCR1_REG7_MASK | PLL_SWCR1_REG8_MASK;
	val = entry->reg5 << PLL_SWCR1_REG5_OFF;
	val |= entry->reg6 << PLL_SWCR1_REG6_OFF;
	val |= entry->reg7 << PLL_SWCR1_REG7_OFF;
	val |= entry->reg8 << PLL_SWCR1_REG8_OFF;
	ccu_update(swcr1, common, mask, val);

	mask = PLL_SWCR3_DIV_INT_MASK | PLL_SWCR3_DIV_FRC_MASK;
	val = entry->div_int << PLL_SWCR3_DIV_INT_OFF;
	val |= entry->div_frac << PLL_SWCR3_DIV_FRC_OFF;
	ccu_update(swcr3, common, mask, val);

	return 0;
}

static unsigned long ccu_pll_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	return ccu_pll_get_vco_freq(hw);
}

static long ccu_pll_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_pll_config *params = &p->pll;
	unsigned long max_rate = 0;
	unsigned int i;

	for (i = 0; i < params->tbl_size; i++) {
		if (params->rate_tbl[i].rate <= rate) {
			if (max_rate < params->rate_tbl[i].rate)
				max_rate = params->rate_tbl[i].rate;
		}
	}

	return MAX(max_rate, PLL_MIN_FREQ);
}

const struct clk_ops spacemit_ccu_pll_ops = {
	.enable		= ccu_pll_enable,
	.disable	= ccu_pll_disable,
	.set_rate	= ccu_pll_set_rate,
	.recalc_rate	= ccu_pll_recalc_rate,
	.round_rate	= ccu_pll_round_rate,
	.is_enabled	= ccu_pll_is_enabled,
};


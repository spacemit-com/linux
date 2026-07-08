// SPDX-License-Identifier: GPL-2.0-only
/*
 * Spacemit Generic power domain support.
 *
 * Copyright (c) 2026 SpacemiT Technology Co. Ltd
 */

#include <linux/io.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <dt-bindings/power/spacemit,k1-power.h>
#include <dt-bindings/power/spacemit,k3-power.h>

#define APMU_POWER_STATUS_REG	0xf0

struct spacemit_pm_domain_param {
	int reg_pwr_ctrl;
	int bit_hw_mode;
	int bit_sleep2;
	int bit_sleep1;
	int bit_isolation;
	int bit_auto_pwr_on;
	int bit_hw_pwr_stat;
	int bit_pwr_stat;
	int use_hw;
	const char *name;
};

struct spacemit_pm_domain {
	struct generic_pm_domain genpd;
	int pm_index;
	const struct spacemit_pm_domain_param *param;
};

struct spacemit_pmu {
	struct device *dev;
	struct genpd_onecell_data genpd_data;
	struct regmap *regmap;
	struct spacemit_pm_domain **domains;
	int num_domains;
};

struct spacemit_pm_of_data {
	int num_domains;
	const struct spacemit_pm_domain_param *param;
};

static struct spacemit_pmu *gpmu;

static int spacemit_pd_power_off(struct generic_pm_domain *domain)
{
	unsigned int val;
	int loop;
	struct spacemit_pm_domain *spd = container_of(domain, struct spacemit_pm_domain, genpd);
	const struct spacemit_pm_domain_param *p = spd->param;

	if (!spd->param->use_hw) {
		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val &= ~(1 << p->bit_isolation);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(10, 15);

		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val &= ~((1 << p->bit_sleep1) | (1 << p->bit_sleep2));
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(10, 15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << p->bit_pwr_stat)) == 0)
				break;
			usleep_range(4, 6);
		}
	} else {
		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val &= ~(1 << p->bit_auto_pwr_on);
		val &= ~(1 << p->bit_hw_mode);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(10, 30);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << p->bit_hw_pwr_stat)) == 0)
				break;
			usleep_range(4, 6);
		}
	}

	if (loop < 0) {
		dev_err(&domain->dev, "Fail to power-off domain: %d\n", spd->pm_index);
		return -EBUSY;
	}

	return 0;
}

static int spacemit_pd_power_on(struct generic_pm_domain *domain)
{
	int loop;
	unsigned int val;
	struct spacemit_pm_domain *spd = container_of(domain, struct spacemit_pm_domain, genpd);
	const struct spacemit_pm_domain_param *p = spd->param;

	regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
	if (val & (1 << p->bit_pwr_stat)) {
		if (!p->use_hw) {
			regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
			val &= ~(1 << p->bit_isolation);
			regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

			usleep_range(10, 15);

			regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
			val &= ~((1 << p->bit_sleep1) | (1 << p->bit_sleep2));
			regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

			usleep_range(10, 15);

			for (loop = 10000; loop >= 0; --loop) {
				regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
				if ((val & (1 << p->bit_pwr_stat)) == 0)
					break;
				usleep_range(4, 6);
			}
		} else {
			regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
			val &= ~(1 << p->bit_auto_pwr_on);
			val &= ~(1 << p->bit_hw_mode);
			regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

			usleep_range(10, 30);

			for (loop = 10000; loop >= 0; --loop) {
				regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
				if ((val & (1 << p->bit_hw_pwr_stat)) == 0)
					break;
				usleep_range(4, 6);
			}
		}

		if (loop < 0) {
			dev_err(&domain->dev, "power-off domain: %d, error\n", spd->pm_index);
			return -EBUSY;
		}
	}

	if (!p->use_hw) {
		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val |= (1 << p->bit_sleep1);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(20, 25);

		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val |= (1 << p->bit_sleep2) | (1 << p->bit_sleep1);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(20, 25);

		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val |= (1 << p->bit_isolation);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(10, 15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
			if (val & (1 << p->bit_pwr_stat))
				break;
			usleep_range(4, 6);
		}
	} else {
		regmap_read(gpmu->regmap, p->reg_pwr_ctrl, &val);
		val |= (1 << p->bit_auto_pwr_on);
		val |= (1 << p->bit_hw_mode);
		regmap_write(gpmu->regmap, p->reg_pwr_ctrl, val);

		usleep_range(290, 310);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap, APMU_POWER_STATUS_REG, &val);
			if (val & (1 << p->bit_hw_pwr_stat))
				break;
			usleep_range(4, 6);
		}
	}

	if (loop < 0) {
		dev_err(&domain->dev, "power-on domain: %d, error\n", spd->pm_index);
		return -EBUSY;
	}

	return 0;
}

static bool spacemit_pm_get_state(struct spacemit_pmu *pmu,
				  struct spacemit_pm_domain *pd)
{
	u32 reg;

	regmap_read(pmu->regmap, APMU_POWER_STATUS_REG, &reg);

	return reg & (1 << pd->param->bit_pwr_stat);
}

static int spacemit_pm_add_one_domain(struct spacemit_pmu *pmu, int id,
				      const struct spacemit_pm_domain_param *param)
{
	struct spacemit_pm_domain *pd;

	pd = devm_kzalloc(pmu->dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	pd->pm_index        = id;
	pd->param           = param;
	pd->genpd.name      = param->name;
	pd->genpd.power_off = spacemit_pd_power_off;
	pd->genpd.power_on  = spacemit_pd_power_on;

	pm_genpd_init(&pd->genpd, NULL, !spacemit_pm_get_state(pmu, pd));

	pmu->domains[id] = pd;

	return 0;
}

static void spacemit_pm_domain_cleanup(struct spacemit_pmu *pmu)
{
	int i;

	for (i = 0; i < pmu->num_domains; i++) {
		if (pmu->domains[i])
			pm_genpd_remove(&pmu->domains[i]->genpd);
	}
}

static int spacemit_pm_domain_probe(struct platform_device *pdev)
{
	const struct spacemit_pm_of_data *data;
	struct device *dev = &pdev->dev;
	struct spacemit_pmu *pmu;
	int err, i;

	data = device_get_match_data(dev);

	pmu = devm_kzalloc(dev, sizeof(*pmu), GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->dev = dev;
	pmu->num_domains = data->num_domains;

	pmu->regmap = syscon_regmap_lookup_by_phandle(dev->of_node, "spacemit,apmu");
	if (IS_ERR(pmu->regmap)) {
		dev_err(dev, "failed to get apmu regmap\n");
		return PTR_ERR(pmu->regmap);
	}

	pmu->domains = devm_kcalloc(dev, data->num_domains,
				    sizeof(*pmu->domains), GFP_KERNEL);
	if (!pmu->domains)
		return -ENOMEM;

	for (i = 0; i < data->num_domains; i++) {
		err = spacemit_pm_add_one_domain(pmu, i, &data->param[i]);
		if (err) {
			dev_err(dev, "failed to add domain %d: %d\n", i, err);
			goto err_out;
		}
	}

	pmu->genpd_data.domains     = (struct generic_pm_domain **)pmu->domains;
	pmu->genpd_data.num_domains = data->num_domains;

	err = of_genpd_add_provider_onecell(dev->of_node, &pmu->genpd_data);
	if (err) {
		dev_err(dev, "failed to add provider: %d\n", err);
		goto err_out;
	}

	gpmu = pmu;

	return 0;

err_out:
	spacemit_pm_domain_cleanup(pmu);
	return err;
}

static const struct spacemit_pm_domain_param k1_domain_params[] = {
	[K1_PMDOMAIN_VPU] = {
		.reg_pwr_ctrl		= 0xa8,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_pwr_stat		= 1,
		.bit_hw_pwr_stat	= 9,
		.name			= "vpu",
	},
	[K1_PMDOMAIN_GPU] = {
		.reg_pwr_ctrl		= 0xd0,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_pwr_stat		= 0,
		.name			= "gpu",
	},
	[K1_PMDOMAIN_LCD] = {
		.reg_pwr_ctrl		= 0x380,
		.bit_hw_mode		= 4,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 4,
		.bit_hw_pwr_stat	= 12,
		.use_hw			= 1,
		.name			= "lcd",
	},
	[K1_PMDOMAIN_ISP] = {
		.reg_pwr_ctrl		= 0x37c,
		.bit_hw_mode		= 4,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 2,
		.bit_hw_pwr_stat	= 10,
		.name			= "isp",
	},
	[K1_PMDOMAIN_AUDIO] = {
		.reg_pwr_ctrl		= 0x378,
		.bit_hw_mode		= 4,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 3,
		.bit_hw_pwr_stat	= 11,
		.use_hw			= 1,
		.name			= "audio",
	},
	[K1_PMDOMAIN_GNSS] = {
		.reg_pwr_ctrl		= 0x13c,
		.bit_hw_mode		= 4,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 6,
		.bit_hw_pwr_stat	= 14,
		.name			= "gnss",
	},
	[K1_PMDOMAIN_HDMI] = {
		.reg_pwr_ctrl		= 0x3f4,
		.bit_hw_mode		= 4,
		.bit_sleep2		= 3,
		.bit_sleep1		= 2,
		.bit_isolation		= 1,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 7,
		.bit_hw_pwr_stat	= 15,
		.use_hw			= 1,
		.name			= "hdmi",
	},
};

static const struct spacemit_pm_domain_param k3_domain_params[] = {
	[K3_PMDOMAIN_VPU] = {
		.bit_auto_pwr_on	= 0,
		.bit_isolation		= 1,
		.bit_sleep1		= 2,
		.bit_sleep2		= 3,
		.bit_hw_mode		= 4,
		.bit_pwr_stat		= 2,
		.bit_hw_pwr_stat	= 9,
		.use_hw			= 1,
		.reg_pwr_ctrl		= 0xa8,
		.name			= "vpu",
	},
	[K3_PMDOMAIN_GPU] = {
		.bit_auto_pwr_on	= 0,
		.bit_isolation		= 1,
		.bit_sleep1		= 2,
		.bit_sleep2		= 3,
		.bit_hw_mode		= 4,
		.bit_pwr_stat		= 0,
		.bit_hw_pwr_stat	= 8,
		.use_hw			= 1,
		.reg_pwr_ctrl		= 0xd0,
		.name			= "gpu",
	},
	[K3_PMDOMAIN_AUDIO] = {
		.bit_auto_pwr_on	= 0,
		.bit_isolation		= 1,
		.bit_sleep1		= 2,
		.bit_sleep2		= 3,
		.bit_hw_mode		= 4,
		.bit_pwr_stat		= 3,
		.reg_pwr_ctrl		= 0x378,
		.name			= "audio",
	},
	[K3_PMDOMAIN_LCD0] = {
		.bit_auto_pwr_on	= 0,
		.bit_isolation		= 1,
		.bit_sleep1		= 2,
		.bit_sleep2		= 3,
		.bit_hw_mode		= 4,
		.bit_pwr_stat		= 4,
		.bit_hw_pwr_stat	= 12,
		.use_hw			= 1,
		.reg_pwr_ctrl		= 0x380,
		.name			= "lcd0",
	},
	[K3_PMDOMAIN_LCD1] = {
		.bit_auto_pwr_on	= 0,
		.bit_isolation		= 1,
		.bit_sleep1		= 2,
		.bit_sleep2		= 3,
		.bit_hw_mode		= 4,
		.bit_pwr_stat		= 5,
		.bit_hw_pwr_stat	= 15,
		.use_hw			= 1,
		.reg_pwr_ctrl		= 0x3f4,
		.name			= "lcd1",
	},
};

static const struct spacemit_pm_of_data k1_of_data = {
	.num_domains	= ARRAY_SIZE(k1_domain_params),
	.param		= k1_domain_params,
};

static const struct spacemit_pm_of_data k3_of_data = {
	.num_domains	= ARRAY_SIZE(k3_domain_params),
	.param		= k3_domain_params,
};

static const struct of_device_id spacemit_pm_domain_dt_match[] = {
	{
		.compatible = "spacemit,k1-power-controller",
		.data = &k1_of_data,
	},
	{
		.compatible = "spacemit,k3-power-controller",
		.data = &k3_of_data,
	},
	{ },
};

static struct platform_driver spacemit_pm_domain_driver = {
	.probe = spacemit_pm_domain_probe,
	.driver = {
		.name           = "spacemit-pm-domain",
		.of_match_table = spacemit_pm_domain_dt_match,
	},
};

builtin_platform_driver(spacemit_pm_domain_driver);

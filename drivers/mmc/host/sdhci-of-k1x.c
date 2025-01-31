// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for Spacemit Mobile Storage Host Controller
 *
 * Copyright (C) 2024 Spacemit
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include "sdhci.h"
#include "sdhci-pltfm.h"

/* SDH registers define */
#define SDHC_OP_EXT_REG			0x108
#define  OVRRD_CLK_OEN			BIT(11)
#define  FORCE_CLK_ON			BIT(12)

#define SDHC_LEGACY_CTRL_REG		0x10C
#define GEN_PAD_CLK_ON			0x0040

#define SDHC_MMC_CTRL_REG		0x114
#define MISC_INT_EN			0x0002
#define MISC_INT			0x0004
#define ENHANCE_STROBE_EN		0x0100
#define MMC_HS400			0x0200
#define MMC_HS200			0x0400
#define MMC_CARD_MODE			0x1000

#define SDHC_TX_CFG_REG			0x11C
#define TX_INT_CLK_SEL			0x40000000
#define TX_MUX_SEL			0x80000000

#define SDHC_PHY_CTRL_REG		0x160
#define  PHY_FUNC_EN			BIT(0)
#define  PHY_PLL_LOCK			BIT(1)
#define  HOST_LEGACY_MODE		BIT(31)

#define SDHC_PHY_FUNC_REG		0x164
#define PHY_TEST_EN			0x0080
#define HS200_USE_RFIFO			0x8000

#define SDHC_PHY_DLLCFG			0x168
#define  DLL_PREDLY_NUM			0x04
#define  DLL_FULLDLY_RANGE		0x10
#define  DLL_VREG_CTRL			0x40
#define  DLL_ENABLE			0x80000000
#define  DLL_REFRESH_SWEN_SHIFT		0x1C
#define  DLL_REFRESH_SW_SHIFT		0x1D

#define SDHC_PHY_DLLCFG1		0x16C
#define  DLL_REG2_CTRL			0x0C
#define  DLL_REG3_CTRL_MASK		0xFF
#define  DLL_REG3_CTRL_SHIFT		0x10
#define  DLL_REG2_CTRL_MASK		0xFF
#define  DLL_REG2_CTRL_SHIFT		0x08
#define  DLL_REG1_CTRL			0x92
#define  DLL_REG1_CTRL_MASK		0xFF
#define  DLL_REG1_CTRL_SHIFT		0x00

#define SDHC_PHY_DLLSTS			0x170
#define  DLL_LOCK_STATE			0x01

#define SDHC_PHY_DLLSTS1		0x174
#define  DLL_MASTER_DELAY_MASK		0xFF
#define  DLL_MASTER_DELAY_SHIFT		0x10

#define SDHC_PHY_PADCFG_REG		0x178
#define  RX_BIAS_CTRL_SHIFT		0x5
#define  PHY_DRIVE_SEL_SHIFT		0x0
#define  PHY_DRIVE_SEL_MASK		0x7
#define  PHY_DRIVE_SEL_DEFAULT		0x4

#define RPM_DELAY			50
#define MAX_74CLK_WAIT_COUNT		100

#define MMC1_IO_V18EN			0x04
#define AKEY_ASFAR			0xBABA
#define AKEY_ASSAR			0xEB10

#define SDHC_RX_CFG_REG			0x118
#define  RX_SDCLK_SEL0_MASK		0x03
#define  RX_SDCLK_SEL0_SHIFT		0x00
#define  RX_SDCLK_SEL0			0x02
#define  RX_SDCLK_SEL1_MASK		0x03
#define  RX_SDCLK_SEL1_SHIFT		0x02
#define  RX_SDCLK_SEL1			0x01

#define SDHC_DLINE_CTRL_REG		0x130
#define  DLINE_PU			0x01
#define  RX_DLINE_CODE_MASK		0xFF
#define  RX_DLINE_CODE_SHIFT		0x10
#define  TX_DLINE_CODE_MASK		0xFF
#define  TX_DLINE_CODE_SHIFT		0x18

#define SDHC_DLINE_CFG_REG		0x134
#define  RX_DLINE_REG_MASK		0xFF
#define  RX_DLINE_REG_SHIFT		0x00
#define  RX_DLINE_GAIN_MASK		0x1
#define  RX_DLINE_GAIN_SHIFT		0x8
#define  RX_DLINE_GAIN			0x1
#define  TX_DLINE_REG_MASK		0xFF
#define  TX_DLINE_REG_SHIFT		0x10

#define SDHC_RX_TUNE_DELAY_MIN		0x0
#define SDHC_RX_TUNE_DELAY_MAX		0xFF
#define SDHC_RX_TUNE_DELAY_STEP		0x1

static struct sdhci_host* sdio_host;

/* MMC Quirks */
/* Support SDH controller on FPGA */
#define SDHCI_QUIRK2_SUPPORT_PHY_BYPASS				(1<<25)
/* Disable scan card at probe phase */
#define SDHCI_QUIRK2_DISABLE_PROBE_CDSCAN			(1<<26)
/* Need to set IO capability by SOC part register */
#define SDHCI_QUIRK2_SET_AIB_MMC				(1<<27)
/* Controller not support phy module */
#define SDHCI_QUIRK2_BROKEN_PHY_MODULE				(1<<28)
/* Controller support encrypt module */
#define SDHCI_QUIRK2_SUPPORT_ENCRYPT				(1<<29)

#define MMC_CAP2_QUIRK_BREAK_SDR104	(1 << 30)


#define CANDIDATE_WIN_NUM 3
#define SELECT_DELAY_NUM 9
#define WINDOW_1ST 0
#define WINDOW_2ND 1
#define WINDOW_3RD 2

#define RX_TUNING_WINDOW_THRESHOLD 80
#define RX_TUNING_DLINE_REG 0x09
#define TX_TUNING_DLINE_REG 0x00
#define TX_TUNING_DELAYCODE 127

enum window_type {
	LEFT_WINDOW = 0,
	MIDDLE_WINDOW = 1,
	RIGHT_WINDOW = 2,
};

struct tuning_window {
	u8 type;
	u8 min_delay;
	u8 max_delay;
};

struct rx_tuning {
	u8 rx_dline_reg;
	u8 select_delay_num;
	u8 current_delay_index;
	/* 0: biggest window, 1: bigger, 2:  small */
	struct tuning_window windows[CANDIDATE_WIN_NUM];
	u8 select_delay[SELECT_DELAY_NUM];

	u32 card_cid[4];
	u8 window_limit;
	u8 tuning_fail;
};

/*
 * struct k1x_sdhci_platdata() - Platform device data for Spacemit K1x SDHCI
 * @flags: flags for platform requirement
 * @host_caps: Standard MMC host capabilities bit field
 * @host_caps2: Standard MMC host capabilities bit field
 * @host_caps_disable: Aquila MMC host capabilities disable bit field
 * @host_caps2_disable: Aquila MMC host capabilities disable bit field
 * @quirks: quirks of platform
 * @quirks2: quirks2 of platform
 * @pm_caps: pm_caps of platform
 */
struct k1x_sdhci_platdata {
	u32 host_freq;
	u32 flags;
	u32 host_caps;
	u32 host_caps2;
	u32 host_caps_disable;
	u32 host_caps2_disable;
	u32 quirks;
	u32 quirks2;
	u32 pm_caps;

	u8 tx_dline_reg;
	u8 tx_delaycode;
	u8 phy_driver_sel;
	struct rx_tuning rxtuning;
};

struct sdhci_spacemit {
	struct clk *clk_core;
	struct clk *clk_io;
	struct clk *clk_aib;
	unsigned char power_mode;
	struct pinctrl_state *pin;
	struct pinctrl *pinctrl;
};

#define spacemit_monitor_cmd(cmd) (((cmd) == MMC_READ_SINGLE_BLOCK) || \
				((cmd) == MMC_READ_MULTIPLE_BLOCK) || \
				((cmd) == MMC_WRITE_BLOCK) || \
				((cmd) == MMC_WRITE_MULTIPLE_BLOCK) || \
				((cmd) == MMC_SWITCH) || \
				((cmd) == MMC_ERASE))

static void spacemit_sdhci_reset(struct sdhci_host *host, u8 mask)
{
	struct platform_device *pdev;
	struct k1x_sdhci_platdata *pdata;
	unsigned int reg;

	pdev = to_platform_device(mmc_dev(host->mmc));
	pdata = pdev->dev.platform_data;
	sdhci_reset(host, mask);

	if (mask != SDHCI_RESET_ALL)
		return;

	/* sd/sdio only be SDHCI_QUIRK2_BROKEN_PHY_MODULE */
	if (!(host->quirks2 & SDHCI_QUIRK2_BROKEN_PHY_MODULE)) {
		if (host->quirks2 & SDHCI_QUIRK2_SUPPORT_PHY_BYPASS) {
			/* use phy bypass */
			reg = sdhci_readl(host, SDHC_TX_CFG_REG);
			reg |= TX_INT_CLK_SEL;
			sdhci_writel (host, reg, SDHC_TX_CFG_REG);

			reg = sdhci_readl(host, SDHC_PHY_CTRL_REG);
			reg |= HOST_LEGACY_MODE;
			sdhci_writel (host, reg, SDHC_PHY_CTRL_REG);

			reg = sdhci_readl(host, SDHC_PHY_FUNC_REG);
			reg |= PHY_TEST_EN;
			sdhci_writel (host, reg, SDHC_PHY_FUNC_REG);
		} else {
			/* use phy func mode */
			reg = sdhci_readl(host, SDHC_PHY_CTRL_REG);
			reg |= (PHY_FUNC_EN | PHY_PLL_LOCK);
			sdhci_writel(host, reg, SDHC_PHY_CTRL_REG);

			reg = sdhci_readl(host, SDHC_PHY_PADCFG_REG);
			reg |= (1 << RX_BIAS_CTRL_SHIFT);

			reg &= ~(PHY_DRIVE_SEL_MASK);
			reg |= (pdata->phy_driver_sel & PHY_DRIVE_SEL_MASK) << PHY_DRIVE_SEL_SHIFT;
			sdhci_writel(host, reg, SDHC_PHY_PADCFG_REG);
		}
	} else {
		reg = sdhci_readl(host, SDHC_TX_CFG_REG);
		reg |= TX_INT_CLK_SEL;
		sdhci_writel (host, reg, SDHC_TX_CFG_REG);
	}

	/* for emmc */
	if (!(host->mmc->caps2 & MMC_CAP2_NO_MMC)) {
		/* mmc card mode */
		reg = sdhci_readl(host, SDHC_MMC_CTRL_REG);
		reg |= MMC_CARD_MODE;
		sdhci_writel(host, reg, SDHC_MMC_CTRL_REG);
	}
}

static void spacemit_sdhci_caps_disable(struct sdhci_host *host)
{
	struct platform_device *pdev;
	struct k1x_sdhci_platdata *pdata;

	pdev = to_platform_device(mmc_dev(host->mmc));
	pdata = pdev->dev.platform_data;

	if (pdata->host_caps_disable)
		host->mmc->caps &= ~(pdata->host_caps_disable);
	if (pdata->host_caps2_disable)
		host->mmc->caps2 &= ~(pdata->host_caps2_disable);
}

static void spacemit_sdhci_set_uhs_signaling(struct sdhci_host *host, unsigned timing)
{
	u16 reg;

	if ((timing == MMC_TIMING_MMC_HS200) ||
	    (timing == MMC_TIMING_MMC_HS400)) {
		reg = sdhci_readw(host, SDHC_MMC_CTRL_REG);
		reg |= (timing == MMC_TIMING_MMC_HS200) ? MMC_HS200 : MMC_HS400;
		sdhci_writew(host, reg, SDHC_MMC_CTRL_REG);
	}
	sdhci_set_uhs_signaling(host, timing);
	if (!(host->mmc->caps2 & MMC_CAP2_NO_SDIO)) {
		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		sdhci_writew(host, reg | SDHCI_CTRL_VDD_180, SDHCI_HOST_CONTROL2);
	}
}

static void spacemit_sdhci_set_clk_gate(struct sdhci_host *host, unsigned int auto_gate)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHC_OP_EXT_REG);
	if (auto_gate)
		reg &= ~(OVRRD_CLK_OEN | FORCE_CLK_ON);
	else
		reg |= (OVRRD_CLK_OEN | FORCE_CLK_ON);
	sdhci_writel(host, reg, SDHC_OP_EXT_REG);
}

static void spacemit_sdhci_enable_sdio_irq_nolock(struct sdhci_host *host, int enable)
{
	if (!(host->flags & SDHCI_DEVICE_DEAD)) {
		if (enable)
			host->ier |= SDHCI_INT_CARD_INT;
		else
			host->ier &= ~SDHCI_INT_CARD_INT;

		sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
		sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);
	}
}

static void spacemit_sdhci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sdhci_host *host = mmc_priv(mmc);
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	spacemit_sdhci_enable_sdio_irq_nolock(host, enable);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void spacemit_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sdhci_host *host = mmc_priv(mmc);
	unsigned long flags;

	spacemit_sdhci_enable_sdio_irq(mmc, enable);

	/* avoid to read the SDIO_CCCR_INTx */
	spin_lock_irqsave(&host->lock, flags);
	mmc->sdio_irq_pending = true;
	spin_unlock_irqrestore(&host->lock, flags);
}

static void spacemit_sdhci_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_spacemit *spacemit = sdhci_pltfm_priv(pltfm_host);
	struct mmc_host *mmc = host->mmc;
	unsigned int reg;
	u32 cmd;

	/* according to the SDHC_TX_CFG_REG(0x11c<bit>),
	 * set TX_INT_CLK_SEL to gurantee the hold time
	 * at default speed mode or HS/SDR12/SDR25/SDR50 mode.
	 */
	reg = sdhci_readl(host, SDHC_TX_CFG_REG);
	if ((mmc->ios.timing == MMC_TIMING_LEGACY) ||
	    (mmc->ios.timing == MMC_TIMING_SD_HS) ||
	    (mmc->ios.timing == MMC_TIMING_UHS_SDR12) ||
	    (mmc->ios.timing == MMC_TIMING_UHS_SDR25) ||
	    (mmc->ios.timing == MMC_TIMING_UHS_SDR50) ||
	    (mmc->ios.timing == MMC_TIMING_MMC_HS)) {
		reg |= TX_INT_CLK_SEL;
	} else {
		reg &= ~TX_INT_CLK_SEL;
	}
	sdhci_writel(host, reg, SDHC_TX_CFG_REG);

	/* set pinctrl state */
	if (spacemit->pinctrl && !IS_ERR(spacemit->pinctrl)) {
		if (clock >= 200000000) {
			spacemit->pin = pinctrl_lookup_state(spacemit->pinctrl, "fast");
			if (IS_ERR(spacemit->pin))
				pr_warn("could not get sdhci pinctrl state.\n");
			else
				pinctrl_select_state(spacemit->pinctrl, spacemit->pin);

		} else {
			spacemit->pin = pinctrl_lookup_state(spacemit->pinctrl, "default");
			if (IS_ERR(spacemit->pin))
				pr_warn("could not get sdhci pinctrl state.\n");
			else
				pinctrl_select_state(spacemit->pinctrl, spacemit->pin);
		}
	}

	if (host->mmc->caps2 & MMC_CAP2_NO_MMC) {
		/*
		* according to the SD spec, during a signal voltage level switch,
		* the clock must be closed for 5 ms.
		* then, the host starts providing clk at 1.8 and the host checks whether
		* DAT[3:0] is high after 1ms clk.
		*
		* for the above goal, temporarily disable the auto clk and keep clk always on for 1ms.
		*/
		cmd = SDHCI_GET_CMD(sdhci_readw(host, SDHCI_COMMAND));
		if ((cmd == SD_SWITCH_VOLTAGE) && (host->mmc->ios.signal_voltage == MMC_SIGNAL_VOLTAGE_180)) {
			/* disable auto clock */
			spacemit_sdhci_set_clk_gate(host, 0);
		}
	}

	sdhci_set_clock(host, clock);
};

static void spacemit_sdhci_phy_dll_init(struct sdhci_host *host)
{
	u32 reg;
	int i;

	/* config dll_reg1 & dll_reg2 */
	reg = sdhci_readl(host, SDHC_PHY_DLLCFG);
	reg |= (DLL_PREDLY_NUM | DLL_FULLDLY_RANGE | DLL_VREG_CTRL);
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG);

	reg = sdhci_readl(host, SDHC_PHY_DLLCFG1);
	reg |= (DLL_REG1_CTRL & DLL_REG1_CTRL_MASK);
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG1);

	/* dll enable */
	reg = sdhci_readl(host, SDHC_PHY_DLLCFG);
	reg |= DLL_ENABLE;
	sdhci_writel(host, reg, SDHC_PHY_DLLCFG);

	/* wait dll lock */
	i = 0;
	while (i++ < 100) {
		if (sdhci_readl(host, SDHC_PHY_DLLSTS) & DLL_LOCK_STATE)
			break;
		udelay(10);
	}
	if (i == 100)
		pr_err("%s: dll lock timeout\n", mmc_hostname(host->mmc));
}

static void spacemit_sdhci_hs400_enhanced_strobe(struct mmc_host *mmc,
					struct mmc_ios *ios)
{
	u32 reg;
	struct sdhci_host *host = mmc_priv(mmc);

	reg = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	if (ios->enhanced_strobe)
		reg |= ENHANCE_STROBE_EN;
	else
		reg &= ~ENHANCE_STROBE_EN;
	sdhci_writel(host, reg, SDHC_MMC_CTRL_REG);

	if (ios->enhanced_strobe)
		spacemit_sdhci_phy_dll_init(host);
}

static unsigned int spacemit_sdhci_clk_get_max_clock(struct sdhci_host *host)
{
	unsigned long rate;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	rate = clk_get_rate(pltfm_host->clk);
	return rate;
}

static unsigned int spacemit_get_max_timeout_count(struct sdhci_host *host)
{
	/*
	 * the default sdhci code use the 1 << 27 as the max timeout counter
	 * to calculate the max_busy_timeout.
	 * aquilac sdhci support 1 << 29 as the timeout counter.
	 */
	return 1 << 29;
}

static int spacemit_sdhci_pre_select_hs400(struct mmc_host *mmc)
{
	u32 reg;
	struct sdhci_host *host = mmc_priv(mmc);

	reg = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	reg |= MMC_HS400;
	sdhci_writel(host, reg, SDHC_MMC_CTRL_REG);
	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;

	return 0;
}

static void spacemit_sdhci_post_select_hs400(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);

	spacemit_sdhci_phy_dll_init(host);
	host->mmc->caps &= ~MMC_CAP_WAIT_WHILE_BUSY;
}

static void spacemit_sdhci_pre_hs400_to_hs200(struct mmc_host *mmc)
{
	u32 reg;
	struct sdhci_host *host = mmc_priv(mmc);

	reg = sdhci_readl(host, SDHC_PHY_CTRL_REG);
	reg &= ~(PHY_FUNC_EN | PHY_PLL_LOCK);
	sdhci_writel(host, reg, SDHC_PHY_CTRL_REG);

	reg = sdhci_readl(host, SDHC_MMC_CTRL_REG);
	reg &= ~(MMC_HS400 | MMC_HS200 | ENHANCE_STROBE_EN);
	sdhci_writel(host, reg, SDHC_MMC_CTRL_REG);

	reg = sdhci_readl(host, SDHC_PHY_FUNC_REG);
	reg &= ~HS200_USE_RFIFO;
	sdhci_writel(host, reg, SDHC_PHY_FUNC_REG);

	udelay(5);

	reg = sdhci_readl(host, SDHC_PHY_CTRL_REG);
	reg |= (PHY_FUNC_EN | PHY_PLL_LOCK);
	sdhci_writel(host, reg, SDHC_PHY_CTRL_REG);
}

static const struct sdhci_ops spacemit_sdhci_ops = {
	.set_clock		= spacemit_sdhci_set_clock,
	.get_max_clock		= spacemit_sdhci_clk_get_max_clock,
	.get_max_timeout_count	= spacemit_get_max_timeout_count,
	.set_bus_width		= sdhci_set_bus_width,
	.reset			= spacemit_sdhci_reset,
	.set_uhs_signaling	= spacemit_sdhci_set_uhs_signaling,
};

static const struct sdhci_pltfm_data sdhci_k1x_pdata = {
	.ops = &spacemit_sdhci_ops,
	.quirks = SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK
		| SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC
		| SDHCI_QUIRK_32BIT_ADMA_SIZE
		| SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN
		| SDHCI_QUIRK_BROKEN_CARD_DETECTION
		| SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
	.quirks2 = SDHCI_QUIRK2_BROKEN_64_BIT_DMA
		| SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};

static const struct of_device_id sdhci_spacemit_of_match[] = {
	{ .compatible = "spacemit,k1-x-sdhci", .data = &sdhci_k1x_pdata },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sdhci_spacemit_of_match);

static struct k1x_sdhci_platdata *spacemit_get_mmc_pdata(struct device *dev)
{
	struct k1x_sdhci_platdata *pdata;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return NULL;
	return pdata;
}

static void spacemit_get_of_property(struct sdhci_host *host,
		struct device *dev, struct k1x_sdhci_platdata *pdata)
{
	struct device_node *np = dev->of_node;
	u32 property;

	/* sdh io clk */
	if (!of_property_read_u32(np, "spacemit,sdh-freq", &property))
		pdata->host_freq = property;

	if (!of_property_read_u32(np, "spacemit,sdh-flags", &property))
		pdata->flags |= property;

	if (!of_property_read_u32(np, "spacemit,sdh-host-caps", &property))
		pdata->host_caps |= property;
	if (!of_property_read_u32(np, "spacemit,sdh-host-caps2", &property))
		pdata->host_caps2 |= property;

	if (!of_property_read_u32(np, "spacemit,sdh-host-caps-disable", &property))
		pdata->host_caps_disable |= property;
	if (!of_property_read_u32(np, "spacemit,sdh-host-caps2-disable", &property))
		pdata->host_caps2_disable |= property;

	if (!of_property_read_u32(np, "spacemit,sdh-quirks", &property))
		pdata->quirks |= property;
	if (!of_property_read_u32(np, "spacemit,sdh-quirks2", &property))
		pdata->quirks2 |= property;

	/* read rx tuning dline_reg */
	if (!of_property_read_u32(np, "spacemit,rx_dline_reg", &property))
		pdata->rxtuning.rx_dline_reg = (u8)property;
	else
		pdata->rxtuning.rx_dline_reg = RX_TUNING_DLINE_REG;

	/* read rx tuning window limit */
	if (!of_property_read_u32(np, "spacemit,rx_tuning_limit", &property))
		pdata->rxtuning.window_limit = (u8)property;
	else
		pdata->rxtuning.window_limit = RX_TUNING_WINDOW_THRESHOLD;

	/* tx tuning dline_reg */
	if (!of_property_read_u32(np, "spacemit,tx_dline_reg", &property))
		pdata->tx_dline_reg = (u8)property;
	else
		pdata->tx_dline_reg = TX_TUNING_DLINE_REG;
	if (!of_property_read_u32(np, "spacemit,tx_delaycode", &property))
		pdata->tx_delaycode = (u8)property;
	else
		pdata->tx_delaycode = TX_TUNING_DELAYCODE;

	/* phy driver select */
	if (!of_property_read_u32(np, "spacemit,phy_driver_sel", &property))
		pdata->phy_driver_sel = (u8)property;
	else
		pdata->phy_driver_sel = PHY_DRIVE_SEL_DEFAULT;

	return;
}

static int spacemit_sdhci_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct device *dev = &pdev->dev;
	struct sdhci_host *host;
	const struct of_device_id *match;
	struct sdhci_spacemit *spacemit;
	struct k1x_sdhci_platdata *pdata;
	int ret;

	host = sdhci_pltfm_init(pdev, &sdhci_k1x_pdata, sizeof(*spacemit));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);

	spacemit = sdhci_pltfm_priv(pltfm_host);

	spacemit->clk_io = devm_clk_get_enabled(dev, "sdh-io");
	if (IS_ERR(spacemit->clk_io)) {
		ret = PTR_ERR(spacemit->clk_io);
		goto err_clk_get;
	}
	pltfm_host->clk = spacemit->clk_io;

	spacemit->clk_core = devm_clk_get_enabled(dev, "sdh-core");
	if (IS_ERR(spacemit->clk_core)) {
		ret = PTR_ERR(spacemit->clk_core);
		goto err_clk_get;
	}

	spacemit->clk_aib = devm_clk_get_optional_enabled(dev, "aib-clk");

	match = of_match_device(of_match_ptr(sdhci_spacemit_of_match), &pdev->dev);
	if (match) {
		ret = mmc_of_parse(host->mmc);
		if (ret)
			goto err_clk_get;
		sdhci_get_of_property(pdev);
	}

	pdata = pdev->dev.platform_data ? pdev->dev.platform_data : spacemit_get_mmc_pdata(dev);
	if (IS_ERR_OR_NULL(pdata)) {
		goto err_clk_get;
	}

	spacemit_get_of_property(host, dev, pdata);
	if (pdata->quirks)
		host->quirks |= pdata->quirks;
	if (pdata->quirks2)
		host->quirks2 |= pdata->quirks2;
	if (pdata->host_caps)
		host->mmc->caps |= pdata->host_caps;
	if (pdata->host_caps2)
		host->mmc->caps2 |= pdata->host_caps2;
	if (pdata->pm_caps)
		host->mmc->pm_caps |= pdata->pm_caps;
	pdev->dev.platform_data = pdata;

	if (host->mmc->pm_caps)
		host->mmc->pm_flags |= host->mmc->pm_caps;

	if (!(host->mmc->caps2 & MMC_CAP2_NO_MMC)) {
		host->mmc_host_ops.hs400_prepare_ddr = spacemit_sdhci_pre_select_hs400;
		host->mmc_host_ops.hs400_complete = spacemit_sdhci_post_select_hs400;
		host->mmc_host_ops.hs400_downgrade = spacemit_sdhci_pre_hs400_to_hs200;
		if (host->mmc->caps2 & MMC_CAP2_HS400_ES)
			host->mmc_host_ops.hs400_enhanced_strobe = spacemit_sdhci_hs400_enhanced_strobe;
	}

	host->mmc_host_ops.enable_sdio_irq = spacemit_enable_sdio_irq;

	if (!(host->mmc->caps2 & MMC_CAP2_NO_SDIO)) {
		/* skip auto rescan */
		host->mmc->rescan_entered = 1;
	}
	host->mmc->caps |= MMC_CAP_NEED_RSP_BUSY;

	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, RPM_DELAY);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, 1);
	pm_runtime_get_sync(&pdev->dev);

	/* set io clock rate */
	if (pdata->host_freq) {
		ret = clk_set_rate(spacemit->clk_io, pdata->host_freq);
		if (ret) {
			dev_err(dev, "failed to set io clock freq\n");
			goto err_add_host;
		}
	} else {
		dev_err(dev, "failed to get io clock freq\n");
		goto err_add_host;
	}

	ret = sdhci_add_host(host);
	if (ret) {
		dev_err(&pdev->dev, "failed to add spacemit sdhc.\n");
		goto err_add_host;
	} else {
		if (!(host->mmc->caps2 & MMC_CAP2_NO_SDIO)) {
			pr_notice("sdio: save sdio_host <- %p\n", host);
			sdio_host = host;
		}
	}

	spacemit_sdhci_caps_disable(host);

	if ((host->mmc->caps2 & MMC_CAP2_NO_MMC) || (host->quirks2 & SDHCI_QUIRK2_BROKEN_PHY_MODULE)) {

		spacemit->pinctrl = devm_pinctrl_get(&pdev->dev);
	}

	if (host->mmc->pm_caps & MMC_PM_WAKE_SDIO_IRQ)
		device_init_wakeup(&pdev->dev, 1);
	pm_runtime_put_autosuspend(&pdev->dev);
	return 0;

err_add_host:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);
err_clk_get:
	sdhci_pltfm_free(pdev);
	return ret;
}

static void spacemit_sdhci_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_spacemit *spacemit = sdhci_pltfm_priv(pltfm_host);

	pm_runtime_get_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);
	sdhci_remove_host(host, 1);

	if (!IS_ERR(spacemit->clk_aib))
		clk_disable_unprepare(spacemit->clk_aib);
	clk_disable_unprepare(spacemit->clk_io);
	clk_disable_unprepare(spacemit->clk_core);

	sdhci_pltfm_free(pdev);
}

static struct platform_driver spacemit_sdhci_driver = {
	.driver		= {
		.name	= "sdhci-spacemit",
		.of_match_table = of_match_ptr(sdhci_spacemit_of_match),
	},
	.probe		= spacemit_sdhci_probe,
	.remove		= spacemit_sdhci_remove,
};

module_platform_driver(spacemit_sdhci_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Spacemit");
MODULE_LICENSE("GPL v2");

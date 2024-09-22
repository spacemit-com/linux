// SPDX-License-Identifier: GPL-2.0
/*
 * Spacemit k1x emac ptp driver
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/in.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/of_irq.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/timer.h>
#include <linux/time64.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "k1x-emac.h"

/* for ptp event message , udp port is 319 */
#define DEFAULT_UDP_PORT		(0x13F)

/* ptp ethernet type */
#define DEFAULT_ETH_TYPE		(0x88F7)


void emac_hw_timestamp_config(void __iomem *ioaddr, u32 enable, u8 rx_ptp_type, u32 ptp_msg_id)
{
	u32 val;

	if (enable) {
		/*
		 * enable tx/rx timestamp and config rx ptp type
		 */
		val = TX_TIMESTAMP_EN | RX_TIMESTAMP_EN;
		val |= (rx_ptp_type << RX_PTP_PKT_TYPE_OFST) & RX_PTP_PKT_TYPE_MSK;
		writel(val, ioaddr + PTP_1588_CTRL);

		/* config ptp message id */
		writel(ptp_msg_id, ioaddr + PTP_MSG_ID);

		/* config ptp ethernet type */
		writel(DEFAULT_ETH_TYPE, ioaddr + PTP_ETH_TYPE);

		/* config ptp udp port */
		writel(DEFAULT_UDP_PORT, ioaddr + PTP_UDP_PORT);

	} else
		writel(0, ioaddr + PTP_1588_CTRL);
}

u32 emac_hw_config_systime_increment(void __iomem *ioaddr, u32 ptp_clock, u32 adj_clock)
{
	u32 incr_val;
	u32 incr_period;
	u32 val;
	u32 period = 0, def_period = 0;
	/*
	 *  set system time counter resolution as ns
	 *  if ptp clock is 50Mhz,  20ns per clock cycle,
	 *  so increment value should be 20,
	 *  increment period should be 1m
	 */
	if (ptp_clock == adj_clock) {
		incr_val = div_u64(1000000000ULL, ptp_clock);
		incr_period = 1;
	} else {
		def_period = div_u64(1000000000ULL, ptp_clock);
		period = div_u64(1000000000ULL, adj_clock);
		if (def_period == period)
			return 0;

		incr_period = 1;
		incr_val = (def_period * def_period)/ period;
	}

	val = (incr_val | (incr_period << INCR_PERIOD_OFST));
	writel(val, ioaddr + PTP_INRC_ATTR);

	return 0;
}

int emac_hw_adjust_systime(void __iomem *ioaddr, u32 ns, bool is_neg)
{
	u32 val = 0;

	/* update system time adjust low register */
	writel(ns, ioaddr + SYS_TIME_ADJ_LOW);

	/* perform system time adjust */
	if (is_neg)
		val |= SYS_TIME_IS_NEG;

	writel(val, ioaddr + SYS_TIME_ADJ_HI);
	return 0;
}

u64 emac_hw_get_systime(void __iomem *ioaddr)
{
	u64 lns;
	u64 hns;

	/* first read system time low register */
	lns = readl(ioaddr + SYS_TIME_GET_LOW);
	hns = readl(ioaddr + SYS_TIME_GET_HI);

	return ((hns << 32) | lns);
}

u64 emac_hw_get_tx_timestamp(void __iomem *ioaddr)
{
	u64 lns;
	u64 hns;

	/* first read system time low register */
	lns = readl(ioaddr + TX_TIMESTAMP_LOW);
	hns = readl(ioaddr + TX_TIMESTAMP_HI);

	return ((hns << 32) | lns);
}

u64 emac_hw_get_rx_timestamp(void __iomem *ioaddr)
{
	u64 lns;
	u64 hns;

	/* first read system time low register */
	lns = readl(ioaddr + RX_TIMESTAMP_LOW);
	hns = readl(ioaddr + RX_TIMESTAMP_HI);

	return ((hns << 32) | lns);
}

int emac_hw_init_systime(void __iomem *ioaddr, u64 set_ns)
{
	u64 cur_ns;
	s32 adj;
	int neg_adj = 0;

	cur_ns = emac_hw_get_systime(ioaddr);

	adj = ((set_ns & SYS_TIME_LOW_MSK) - (cur_ns & SYS_TIME_LOW_MSK));
	if (adj < 0) {
		neg_adj = 1;
		adj = -adj;
	}
	/* according to spec , set system time upper register and adjust time */
	writel(set_ns >> 32, ioaddr + SYS_TIME_GET_HI);

	emac_hw_adjust_systime(ioaddr, adj, neg_adj);

	return 0;
}

struct emac_hw_ptp emac_hwptp = {
	.config_hw_tstamping = emac_hw_timestamp_config,
	.config_systime_increment = emac_hw_config_systime_increment,
	.init_systime = emac_hw_init_systime,
	.adjust_systime = emac_hw_adjust_systime,
	.get_systime = emac_hw_get_systime,
	.get_tx_timestamp = emac_hw_get_tx_timestamp,
	.get_rx_timestamp = emac_hw_get_rx_timestamp,
};

/**
 * emac_adjust_freq
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppb: desired period change in parts ber billion
 *
 * Description: this function will adjust the frequency of hardware clock.
 */
static int emac_adjust_freq(struct ptp_clock_info *ptp, long scaled_ppm)
{
	return -EOPNOTSUPP;
#if 0
	struct emac_priv *priv =
	    container_of(ptp, struct emac_priv, ptp_clock_ops);
	unsigned long flags;
	u32 diff, addend;
	int neg_adj = 0;
	u64 adj;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	addend = priv->ptp_clk_rate;
	adj = addend;
	adj *= ppb;

	/*
	 * ppb = (Fnew - F0)/F0
	 * diff = F0 * ppb
	 */

	diff = div_u64(adj, 1000000000ULL);
	addend = neg_adj ? (addend - diff) : (addend + diff);

	spin_lock_irqsave(&priv->ptp_lock, flags);

	priv->hwptp->adjust_systime(priv->iobase, diff, neg_adj);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
#endif
}

/**
 * emac_adjust_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @delta: desired change in nanoseconds
 *
 * Description: this function will shift/adjust the hardware clock time.
 */
static int emac_adjust_time(struct ptp_clock_info *ptp, s64 delta)
{
	struct emac_priv *priv =
	    container_of(ptp, struct emac_priv, ptp_clock_ops);
	unsigned long flags;
	int neg_adj = 0;
	u64 ns;

	if (delta < 0) {
		neg_adj = 1;
		delta = -delta;
	}


	spin_lock_irqsave(&priv->ptp_lock, flags);
	if (delta > SYS_TIME_LOW_MSK) {
		ns = priv->hwptp->get_systime(priv->iobase);
		ns = neg_adj ? (ns - delta) : (ns + delta);
		emac_hw_init_systime(priv->iobase, ns);
	} else {
		priv->hwptp->adjust_systime(priv->iobase, delta, neg_adj);
	}



	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

/**
 * emac_get_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: pointer to hold time/result
 *
 * Description: this function will read the current time from the
 * hardware clock and store it in @ts.
 */
static int emac_get_time(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct emac_priv *priv =
	    container_of(ptp, struct emac_priv, ptp_clock_ops);
	unsigned long flags;
	u64 ns = 0;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	ns = priv->hwptp->get_systime(priv->iobase);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

/**
 * emac_set_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: time value to set
 *
 * Description: this function will set the current time on the
 * hardware clock.
 */
static int emac_set_time(struct ptp_clock_info *ptp,
			   const struct timespec64 *ts)
{
	struct emac_priv *priv =
	    container_of(ptp, struct emac_priv, ptp_clock_ops);
	unsigned long flags;
	u64 set_ns = 0;

	set_ns = timespec64_to_ns(ts);

	spin_lock_irqsave(&priv->ptp_lock, flags);

	priv->hwptp->init_systime(priv->iobase, set_ns);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info emac_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "emac_ptp_clock",
	.max_adj = 100000000,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0,
	.n_pins = 0,
	.pps = 0,
	.adjfine = emac_adjust_freq,
	.adjtime = emac_adjust_time,
	.gettime64 = emac_get_time,
	.settime64 = emac_set_time,
};

/**
 * emac_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
void emac_ptp_register(struct emac_priv *priv)
{
	spin_lock_init(&priv->ptp_lock);
	priv->ptp_clock_ops = emac_ptp_clock_ops;

	priv->ptp_clock = ptp_clock_register(&priv->ptp_clock_ops,
					     NULL);
	if (IS_ERR(priv->ptp_clock)) {
		netdev_err(priv->ndev, "ptp_clock_register failed\n");
		priv->ptp_clock = NULL;
	} else if (priv->ptp_clock)
		netdev_info(priv->ndev, "registered PTP clock\n");

	priv->hwptp = &emac_hwptp;
}

/**
 * emac_ptp_unregister
 * @priv: driver private structure
 * Description: this function will remove/unregister the ptp clock driver
 * from the kernel.
 */
void emac_ptp_unregister(struct emac_priv *priv)
{
	if (priv->ptp_clock) {
		ptp_clock_unregister(priv->ptp_clock);
		priv->ptp_clock = NULL;
		pr_debug("Removed PTP HW clock successfully on %s\n",
			 priv->ndev->name);
	}
	priv->hwptp = NULL;
}

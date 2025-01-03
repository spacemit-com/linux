// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024 Haylen Chu <heylenay@4d2.org>
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_pll.h"
#include "ccu_mix.h"
#include "ccu_ddn.h"

#include <dt-bindings/clock/spacemit,k1-ccu.h>

/*	APBS register offset	*/
/*	pll1	*/
#define APB_SPARE1_REG			0x100
#define APB_SPARE2_REG			0x104
#define APB_SPARE3_REG			0x108
/*	pll2	*/
#define APB_SPARE7_REG			0x118
#define APB_SPARE8_REG			0x11c
#define APB_SPARE9_REG			0x120
/*	pll3	*/
#define APB_SPARE10_REG			0x124
#define APB_SPARE11_REG			0x128
#define APB_SPARE12_REG			0x12c

/* MPMU register offset */
#define MPMU_POSR			0x10
#define POSR_PLL1_LOCK			BIT(27)
#define POSR_PLL2_LOCK			BIT(28)
#define POSR_PLL3_LOCK			BIT(29)

#define MPMU_WDTPCR			0x200
#define MPMU_RIPCCR			0x210
#define MPMU_ACGR			0x1024
#define MPMU_SUCCR			0x14
#define MPMU_ISCCR			0x44
#define MPMU_SUCCR_1			0x10b0
#define MPMU_APBCSCR			0x1050

/* APBC register offset */
#define APBC_UART1_CLK_RST		0x0
#define APBC_UART2_CLK_RST		0x4
#define APBC_GPIO_CLK_RST		0x8
#define APBC_PWM0_CLK_RST		0xc
#define APBC_PWM1_CLK_RST		0x10
#define APBC_PWM2_CLK_RST		0x14
#define APBC_PWM3_CLK_RST		0x18
#define APBC_TWSI8_CLK_RST		0x20
#define APBC_UART3_CLK_RST		0x24
#define APBC_RTC_CLK_RST		0x28
#define APBC_TWSI0_CLK_RST		0x2c
#define APBC_TWSI1_CLK_RST		0x30
#define APBC_TIMERS1_CLK_RST		0x34
#define APBC_TWSI2_CLK_RST		0x38
#define APBC_AIB_CLK_RST		0x3c
#define APBC_TWSI4_CLK_RST		0x40
#define APBC_TIMERS2_CLK_RST		0x44
#define APBC_ONEWIRE_CLK_RST		0x48
#define APBC_TWSI5_CLK_RST		0x4c
#define APBC_DRO_CLK_RST		0x58
#define APBC_IR_CLK_RST			0x5c
#define APBC_TWSI6_CLK_RST		0x60
#define APBC_COUNTER_CLK_SEL		0x64
#define APBC_TWSI7_CLK_RST		0x68
#define APBC_TSEN_CLK_RST		0x6c
#define APBC_UART4_CLK_RST		0x70
#define APBC_UART5_CLK_RST		0x74
#define APBC_UART6_CLK_RST		0x78
#define APBC_SSP3_CLK_RST		0x7c
#define APBC_SSPA0_CLK_RST		0x80
#define APBC_SSPA1_CLK_RST		0x84
#define APBC_IPC_AP2AUD_CLK_RST		0x90
#define APBC_UART7_CLK_RST		0x94
#define APBC_UART8_CLK_RST		0x98
#define APBC_UART9_CLK_RST		0x9c
#define APBC_CAN0_CLK_RST		0xa0
#define APBC_PWM4_CLK_RST		0xa8
#define APBC_PWM5_CLK_RST		0xac
#define APBC_PWM6_CLK_RST		0xb0
#define APBC_PWM7_CLK_RST		0xb4
#define APBC_PWM8_CLK_RST		0xb8
#define APBC_PWM9_CLK_RST		0xbc
#define APBC_PWM10_CLK_RST		0xc0
#define APBC_PWM11_CLK_RST		0xc4
#define APBC_PWM12_CLK_RST		0xc8
#define APBC_PWM13_CLK_RST		0xcc
#define APBC_PWM14_CLK_RST		0xd0
#define APBC_PWM15_CLK_RST		0xd4
#define APBC_PWM16_CLK_RST		0xd8
#define APBC_PWM17_CLK_RST		0xdc
#define APBC_PWM18_CLK_RST		0xe0
#define APBC_PWM19_CLK_RST		0xe4

/* APMU register offset */
#define APMU_CCI550_CLK_CTRL		0x300
#define APMU_CPU_C0_CLK_CTRL		0x38C
#define APMU_CPU_C1_CLK_CTRL		0x390
#define APMU_JPG_CLK_RES_CTRL		0x20
#define APMU_CSI_CCIC2_CLK_RES_CTRL	0x24
#define APMU_ISP_CLK_RES_CTRL		0x38
#define APMU_LCD_CLK_RES_CTRL1		0x44
#define APMU_LCD_SPI_CLK_RES_CTRL	0x48
#define APMU_LCD_CLK_RES_CTRL2		0x4c
#define APMU_CCIC_CLK_RES_CTRL		0x50
#define APMU_SDH0_CLK_RES_CTRL		0x54
#define APMU_SDH1_CLK_RES_CTRL		0x58
#define APMU_USB_CLK_RES_CTRL		0x5c
#define APMU_QSPI_CLK_RES_CTRL		0x60
#define APMU_USB_CLK_RES_CTRL		0x5c
#define APMU_DMA_CLK_RES_CTRL		0x64
#define APMU_AES_CLK_RES_CTRL		0x68
#define APMU_VPU_CLK_RES_CTRL		0xa4
#define APMU_GPU_CLK_RES_CTRL		0xcc
#define APMU_SDH2_CLK_RES_CTRL		0xe0
#define APMU_PMUA_MC_CTRL		0xe8
#define APMU_PMU_CC2_AP			0x100
#define APMU_PMUA_EM_CLK_RES_CTRL	0x104
#define APMU_AUDIO_CLK_RES_CTRL		0x14c
#define APMU_HDMI_CLK_RES_CTRL		0x1B8
#define APMU_CCI550_CLK_CTRL		0x300
#define APMU_ACLK_CLK_CTRL		0x388
#define APMU_CPU_C0_CLK_CTRL		0x38C
#define APMU_CPU_C1_CLK_CTRL		0x390
#define APMU_PCIE_CLK_RES_CTRL_0	0x3cc
#define APMU_PCIE_CLK_RES_CTRL_1	0x3d4
#define APMU_PCIE_CLK_RES_CTRL_2	0x3dc
#define APMU_EMAC0_CLK_RES_CTRL		0x3e4
#define APMU_EMAC1_CLK_RES_CTRL		0x3ec

/*	APBS clocks start	*/

/* Frequency of pll{1,2} should not be updated at runtime */
static const struct ccu_pll_rate_tbl pll1_rate_tbl[] = {
	CCU_PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
};

static const struct ccu_pll_rate_tbl pll2_rate_tbl[] = {
	CCU_PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
};

static const struct ccu_pll_rate_tbl pll3_rate_tbl[] = {
	CCU_PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	CCU_PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	CCU_PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
};

static CCU_PLL_DEFINE(pll1, "pll1", pll1_rate_tbl,
		      APB_SPARE1_REG, APB_SPARE2_REG, APB_SPARE3_REG,
		      MPMU_POSR, POSR_PLL1_LOCK, CLK_SET_RATE_GATE);
static CCU_PLL_DEFINE(pll2, "pll2", pll2_rate_tbl,
		      APB_SPARE7_REG, APB_SPARE8_REG, APB_SPARE9_REG,
		      MPMU_POSR, POSR_PLL2_LOCK, CLK_SET_RATE_GATE);
static CCU_PLL_DEFINE(pll3, "pll3", pll3_rate_tbl,
		      APB_SPARE10_REG, APB_SPARE11_REG, APB_SPARE12_REG,
		      MPMU_POSR, POSR_PLL3_LOCK, 0);

static CCU_GATE_FACTOR_DEFINE(pll1_d2, "pll1_d2", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(1), BIT(1), 0, 2, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d3, "pll1_d3", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(2), BIT(2), 0, 3, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d4, "pll1_d4", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(3), BIT(3), 0, 4, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d5, "pll1_d5", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(4), BIT(4), 0, 5, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d6, "pll1_d6", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(5), BIT(5), 0, 6, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d7, "pll1_d7", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(6), BIT(6), 0, 7, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d8, "pll1_d8", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(7), BIT(7), 0, 8, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d11_223p4, "pll1_d11_223p4", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(15), BIT(15), 0, 11, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d13_189, "pll1_d13_189", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(16), BIT(16), 0, 13, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d23_106p8, "pll1_d23_106p8", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(20), BIT(20), 0, 23, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d64_38p4, "pll1_d64_38p4", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(0), BIT(0), 0, 64, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_aud_245p7, "pll1_aud_245p7", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(10), BIT(10), 0, 10, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_aud_24p5, "pll1_aud_24p5", CCU_PARENT_HW(pll1),
			      APB_SPARE2_REG,
			      BIT(11), BIT(11), 0, 100, 1, 0);

static CCU_GATE_FACTOR_DEFINE(pll2_d1, "pll2_d1", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(0), BIT(0), 0, 1, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d2, "pll2_d2", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(1), BIT(1), 0, 2, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d3, "pll2_d3", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(2), BIT(2), 0, 3, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d4, "pll2_d4", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(3), BIT(3), 0, 4, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d5, "pll2_d5", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(4), BIT(4), 0, 5, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d6, "pll2_d6", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(5), BIT(5), 0, 6, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d7, "pll2_d7", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(6), BIT(6), 0, 7, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll2_d8, "pll2_d8", CCU_PARENT_HW(pll2),
			      APB_SPARE8_REG,
			      BIT(7), BIT(7), 0, 8, 1, 0);

static CCU_GATE_FACTOR_DEFINE(pll3_d1, "pll3_d1", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(0), BIT(0), 0, 1, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d2, "pll3_d2", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(1), BIT(1), 0, 2, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d3, "pll3_d3", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(2), BIT(2), 0, 3, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d4, "pll3_d4", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(3), BIT(3), 0, 4, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d5, "pll3_d5", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(4), BIT(4), 0, 5, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d6, "pll3_d6", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(5), BIT(5), 0, 6, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d7, "pll3_d7", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(6), BIT(6), 0, 7, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll3_d8, "pll3_d8", CCU_PARENT_HW(pll3),
			      APB_SPARE11_REG,
			      BIT(7), BIT(7), 0, 8, 1, 0);

static CCU_FACTOR_DEFINE(pll3_20, "pll3_20", CCU_PARENT_HW(pll3_d8), 20, 1);
static CCU_FACTOR_DEFINE(pll3_40, "pll3_40", CCU_PARENT_HW(pll3_d8), 10, 1);
static CCU_FACTOR_DEFINE(pll3_80, "pll3_80", CCU_PARENT_HW(pll3_d8), 5, 1);

/*	APBS clocks end		*/

/*	MPMU clocks start	*/
static CCU_GATE_DEFINE(pll1_d8_307p2, "pll1_d8_307p2", CCU_PARENT_HW(pll1_d8),
		       MPMU_ACGR,
		       BIT(13), BIT(13), 0, 0);
static CCU_FACTOR_DEFINE(pll1_d32_76p8, "pll1_d32_76p8", CCU_PARENT_HW(pll1_d8_307p2),
			 4, 1);
static CCU_FACTOR_DEFINE(pll1_d40_61p44, "pll1_d40_61p44", CCU_PARENT_HW(pll1_d8_307p2),
			 5, 1);
static CCU_FACTOR_DEFINE(pll1_d16_153p6, "pll1_d16_153p6", CCU_PARENT_HW(pll1_d8),
			 2, 1);
static CCU_GATE_FACTOR_DEFINE(pll1_d24_102p4, "pll1_d24_102p4", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(12), BIT(12), 0, 3, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d48_51p2, "pll1_d48_51p2", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(7), BIT(7), 0, 6, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d48_51p2_ap, "pll1_d48_51p2_ap", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(11), BIT(11), 0, 6, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_m3d128_57p6, "pll1_m3d128_57p6", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(8), BIT(8), 0, 16, 3, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d96_25p6, "pll1_d96_25p6", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(4), BIT(4), 0, 12, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d192_12p8, "pll1_d192_12p8", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(3), BIT(3), 0, 24, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d192_12p8_wdt, "pll1_d192_12p8_wdt", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(19), BIT(19), 0x0, 24, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d384_6p4, "pll1_d384_6p4", CCU_PARENT_HW(pll1_d8),
			      MPMU_ACGR,
			      BIT(2), BIT(2), 0, 48, 1, 0);
static CCU_FACTOR_DEFINE(pll1_d768_3p2, "pll1_d768_3p2", CCU_PARENT_HW(pll1_d384_6p4),
			 2, 1);
static CCU_FACTOR_DEFINE(pll1_d1536_1p6, "pll1_d1536_1p6", CCU_PARENT_HW(pll1_d384_6p4),
			 4, 1);
static CCU_FACTOR_DEFINE(pll1_d3072_0p8, "pll1_d3072_0p8", CCU_PARENT_HW(pll1_d384_6p4),
			 8, 1);

static CCU_FACTOR_DEFINE(pll1_d7_351p08, "pll1_d7_351p08", CCU_PARENT_HW(pll1_d7),
			 1, 1);

static CCU_GATE_DEFINE(pll1_d6_409p6, "pll1_d6_409p6", CCU_PARENT_HW(pll1_d6),
		       MPMU_ACGR,
		       BIT(0), BIT(0), 0, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d12_204p8, "pll1_d12_204p8", CCU_PARENT_HW(pll1_d6),
			      MPMU_ACGR,
			      BIT(5), BIT(5), 0, 2, 1, 0);

static CCU_GATE_DEFINE(pll1_d5_491p52, "pll1_d5_491p52", CCU_PARENT_HW(pll1_d5),
		       MPMU_ACGR,
		       BIT(21), BIT(21), 0, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d10_245p76, "pll1_d10_245p76", CCU_PARENT_HW(pll1_d5),
			      MPMU_ACGR,
			      BIT(18), BIT(18), 0, 2, 1, 0);

static CCU_GATE_DEFINE(pll1_d4_614p4, "pll1_d4_614p4", CCU_PARENT_HW(pll1_d4),
		       MPMU_ACGR,
		       BIT(15), BIT(15), 0, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d52_47p26, "pll1_d52_47p26", CCU_PARENT_HW(pll1_d4),
			      MPMU_ACGR,
			      BIT(10), BIT(10), 0, 13, 1, 0);
static CCU_GATE_FACTOR_DEFINE(pll1_d78_31p5, "pll1_d78_31p5", CCU_PARENT_HW(pll1_d4),
			      MPMU_ACGR,
			      BIT(6), BIT(6), 0, 39, 2, 0);

static CCU_GATE_DEFINE(pll1_d3_819p2, "pll1_d3_819p2", CCU_PARENT_HW(pll1_d3),
		       MPMU_ACGR,
		       BIT(14), BIT(14), 0, 0);

static CCU_GATE_DEFINE(pll1_d2_1228p8, "pll1_d2_1228p8", CCU_PARENT_HW(pll1_d2),
		       MPMU_ACGR,
		       BIT(16), BIT(16), 0, 0);

static struct ccu_ddn_info uart_ddn_mask_info = {
	.factor		= 2,
	.num_mask	= 0x1fff,
	.den_mask	= 0x1fff,
	.num_shift	= 16,
	.den_shift	= 0,
};
static struct ccu_ddn_tbl slow_uart1_tbl[] = {
	{ .num = 125, .den = 24 },
};
static struct ccu_ddn_tbl slow_uart2_tbl[] = {
	{ .num = 6144, .den = 960 },
};
static CCU_GATE_DEFINE(slow_uart, "slow_uart", CCU_PARENT_NAME(osc),
		       MPMU_ACGR,
		       BIT(1), BIT(1), 0, CLK_IGNORE_UNUSED);
static CCU_DDN_DEFINE(slow_uart1_14p74, "slow_uart1_14p74", pll1_d16_153p6,
		      &uart_ddn_mask_info, slow_uart1_tbl,
		      MPMU_SUCCR, 0);
static CCU_DDN_DEFINE(slow_uart2_48, "slow_uart2_48", pll1_d4_614p4,
		      &uart_ddn_mask_info, slow_uart2_tbl,
		      MPMU_SUCCR_1, 0);

static CCU_GATE_DEFINE(wdt_clk, "wdt_clk", CCU_PARENT_HW(pll1_d96_25p6),
		       MPMU_WDTPCR,
		       BIT(1), BIT(1), 0x0,
		       0);

static CCU_GATE_DEFINE(ripc_clk, "ripc_clk", CCU_PARENT_NAME(vctcxo_24m),
		       MPMU_RIPCCR,
		       0x3, 0x3, 0x0,
		       0);

static CCU_GATE_FACTOR_DEFINE(i2s_sysclk, "i2s_sysclk", CCU_PARENT_HW(pll1_d16_153p6),
			      MPMU_ISCCR,
			      BIT(31), BIT(31), 0x0, 50, 1,
			      0);
static CCU_GATE_FACTOR_DEFINE(i2s_bclk, "i2s_bclk", CCU_PARENT_HW(i2s_sysclk),
			      MPMU_ISCCR,
			      BIT(29), BIT(29), 0x0, 1, 1,
			      0);

static const struct clk_parent_data apb_parents[] = {
	CCU_PARENT_HW(pll1_d96_25p6),
	CCU_PARENT_HW(pll1_d48_51p2),
	CCU_PARENT_HW(pll1_d96_25p6),
	CCU_PARENT_HW(pll1_d24_102p4),
};
static CCU_MUX_DEFINE(apb_clk, "apb_clk", apb_parents,
		      MPMU_APBCSCR,
		      0, 2,
		      0);

static CCU_GATE_DEFINE(wdt_bus_clk, "wdt_bus_clk", CCU_PARENT_HW(apb_clk),
		       MPMU_WDTPCR,
		       BIT(2), BIT(2), 0x0,
		       0);
/*	MPMU clocks end		*/

/*	APBC clocks start	*/
static const struct clk_parent_data uart_clk_parents[] = {
	CCU_PARENT_HW(pll1_m3d128_57p6),
	CCU_PARENT_HW(slow_uart1_14p74),
	CCU_PARENT_HW(slow_uart2_48),
};
static CCU_MUX_GATE_DEFINE(uart0_clk, "uart0_clk", uart_clk_parents,
			   APBC_UART1_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   CLK_IS_CRITICAL);
static CCU_MUX_GATE_DEFINE(uart2_clk, "uart2_clk", uart_clk_parents,
			   APBC_UART2_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart3_clk, "uart3_clk", uart_clk_parents,
			   APBC_UART3_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart4_clk, "uart4_clk", uart_clk_parents,
			   APBC_UART4_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart5_clk, "uart5_clk", uart_clk_parents,
			   APBC_UART5_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart6_clk, "uart6_clk", uart_clk_parents,
			   APBC_UART6_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart7_clk, "uart7_clk", uart_clk_parents,
			   APBC_UART7_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart8_clk, "uart8_clk", uart_clk_parents,
			   APBC_UART8_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(uart9_clk, "uart9_clk", uart_clk_parents,
			   APBC_UART9_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);

static CCU_GATE_DEFINE(gpio_clk, "gpio_clk", CCU_PARENT_NAME(vctcxo_24m),
		       APBC_GPIO_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);

static const struct clk_parent_data pwm_parents[] = {
	CCU_PARENT_HW(pll1_d192_12p8),
	CCU_PARENT_NAME(osc),
};
static CCU_MUX_GATE_DEFINE(pwm0_clk, "pwm0_clk", pwm_parents,
			   APBC_PWM0_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm1_clk, "pwm1_clk", pwm_parents,
			   APBC_PWM1_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm2_clk, "pwm2_clk", pwm_parents,
			   APBC_PWM2_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm3_clk, "pwm3_clk", pwm_parents,
			   APBC_PWM3_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm4_clk, "pwm4_clk", pwm_parents,
			   APBC_PWM4_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm5_clk, "pwm5_clk", pwm_parents,
			   APBC_PWM5_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm6_clk, "pwm6_clk", pwm_parents,
			   APBC_PWM6_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm7_clk, "pwm7_clk", pwm_parents,
			   APBC_PWM7_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm8_clk, "pwm8_clk", pwm_parents,
			   APBC_PWM8_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm9_clk, "pwm9_clk", pwm_parents,
			   APBC_PWM9_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm10_clk, "pwm10_clk", pwm_parents,
			   APBC_PWM10_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm11_clk, "pwm11_clk", pwm_parents,
			   APBC_PWM11_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm12_clk, "pwm12_clk", pwm_parents,
			   APBC_PWM12_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm13_clk, "pwm13_clk", pwm_parents,
			   APBC_PWM13_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm14_clk, "pwm14_clk", pwm_parents,
			   APBC_PWM14_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm15_clk, "pwm15_clk", pwm_parents,
			   APBC_PWM15_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm16_clk, "pwm16_clk", pwm_parents,
			   APBC_PWM16_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm17_clk, "pwm17_clk", pwm_parents,
			   APBC_PWM17_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm18_clk, "pwm18_clk", pwm_parents,
			   APBC_PWM18_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(pwm19_clk, "pwm19_clk", pwm_parents,
			   APBC_PWM19_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);

static const struct clk_parent_data ssp_parents[] = {
	CCU_PARENT_HW(pll1_d384_6p4),
	CCU_PARENT_HW(pll1_d192_12p8),
	CCU_PARENT_HW(pll1_d96_25p6),
	CCU_PARENT_HW(pll1_d48_51p2),
	CCU_PARENT_HW(pll1_d768_3p2),
	CCU_PARENT_HW(pll1_d1536_1p6),
	CCU_PARENT_HW(pll1_d3072_0p8),
};
static CCU_MUX_GATE_DEFINE(ssp3_clk, "ssp3_clk", ssp_parents,
			   APBC_SSP3_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);

static CCU_GATE_DEFINE(rtc_clk, "rtc_clk", CCU_PARENT_NAME(osc),
		       APBC_RTC_CLK_RST,
		       0x82, 0x82, 0x0,
		       0);

static const struct clk_parent_data twsi_parents[] = {
	CCU_PARENT_HW(pll1_d78_31p5),
	CCU_PARENT_HW(pll1_d48_51p2),
	CCU_PARENT_HW(pll1_d40_61p44),
};
static CCU_MUX_GATE_DEFINE(twsi0_clk, "twsi0_clk", twsi_parents,
			   APBC_TWSI0_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi1_clk, "twsi1_clk", twsi_parents,
			   APBC_TWSI1_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi2_clk, "twsi2_clk", twsi_parents,
			   APBC_TWSI2_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi4_clk, "twsi4_clk", twsi_parents,
			   APBC_TWSI4_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi5_clk, "twsi5_clk", twsi_parents,
			   APBC_TWSI5_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi6_clk, "twsi6_clk", twsi_parents,
			   APBC_TWSI6_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi7_clk, "twsi7_clk", twsi_parents,
			   APBC_TWSI7_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(twsi8_clk, "twsi8_clk", twsi_parents,
			   APBC_TWSI8_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);

static const struct clk_parent_data timer_parents[] = {
	CCU_PARENT_HW(pll1_d192_12p8),
	CCU_PARENT_NAME(osc),
	CCU_PARENT_HW(pll1_d384_6p4),
	CCU_PARENT_NAME(vctcxo_3m),
	CCU_PARENT_NAME(vctcxo_1m),
};
static CCU_MUX_GATE_DEFINE(timers1_clk, "timers1_clk", timer_parents,
			   APBC_TIMERS1_CLK_RST,
			   4, 3, 0x3, 0x3, 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(timers2_clk, "timers2_clk", timer_parents,
			   APBC_TIMERS2_CLK_RST,
			   4, 3, 0x3, 0x3, 0x0,
			   0);

static CCU_GATE_DEFINE(aib_clk, "aib_clk", CCU_PARENT_NAME(vctcxo_24m),
		       APBC_AIB_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);

static CCU_GATE_DEFINE(onewire_clk, "onewire_clk", CCU_PARENT_NAME(vctcxo_24m),
		       APBC_ONEWIRE_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);

static const struct clk_parent_data sspa_parents[] = {
	CCU_PARENT_HW(pll1_d384_6p4),
	CCU_PARENT_HW(pll1_d192_12p8),
	CCU_PARENT_HW(pll1_d96_25p6),
	CCU_PARENT_HW(pll1_d48_51p2),
	CCU_PARENT_HW(pll1_d768_3p2),
	CCU_PARENT_HW(pll1_d1536_1p6),
	CCU_PARENT_HW(pll1_d3072_0p8),
	CCU_PARENT_HW(i2s_bclk),
};
static CCU_MUX_GATE_DEFINE(sspa0_clk, "sspa0_clk", sspa_parents,
			   APBC_SSPA0_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_MUX_GATE_DEFINE(sspa1_clk, "sspa1_clk", sspa_parents,
			   APBC_SSPA1_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_GATE_DEFINE(dro_clk, "dro_clk", CCU_PARENT_HW(apb_clk),
		       APBC_DRO_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);
static CCU_GATE_DEFINE(ir_clk, "ir_clk", CCU_PARENT_HW(apb_clk),
		       APBC_IR_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);
static CCU_GATE_DEFINE(tsen_clk, "tsen_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TSEN_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);
static CCU_GATE_DEFINE(ipc_ap2aud_clk, "ipc_ap2aud_clk", CCU_PARENT_HW(apb_clk),
		       APBC_IPC_AP2AUD_CLK_RST,
		       BIT(1), BIT(1), 0x0,
		       0);

static const struct clk_parent_data can_parents[] = {
	CCU_PARENT_HW(pll3_20),
	CCU_PARENT_HW(pll3_40),
	CCU_PARENT_HW(pll3_80),
};
static CCU_MUX_GATE_DEFINE(can0_clk, "can0_clk", can_parents,
			   APBC_CAN0_CLK_RST,
			   4, 3, BIT(1), BIT(1), 0x0,
			   0);
static CCU_GATE_DEFINE(can0_bus_clk, "can0_bus_clk", CCU_PARENT_NAME(vctcxo_24m),
		       APBC_CAN0_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(uart0_bus_clk, "uart0_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART1_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       CLK_IS_CRITICAL);
static CCU_GATE_DEFINE(uart2_bus_clk, "uart2_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART2_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart3_bus_clk, "uart3_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART3_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart4_bus_clk, "uart4_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART4_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart5_bus_clk, "uart5_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART5_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart6_bus_clk, "uart6_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART6_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart7_bus_clk, "uart7_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART7_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart8_bus_clk, "uart8_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART8_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(uart9_bus_clk, "uart9_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_UART9_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(gpio_bus_clk, "gpio_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_GPIO_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(pwm0_bus_clk, "pwm0_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM0_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm1_bus_clk, "pwm1_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM1_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm2_bus_clk, "pwm2_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM2_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm3_bus_clk, "pwm3_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM3_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm4_bus_clk, "pwm4_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM4_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm5_bus_clk, "pwm5_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM5_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm6_bus_clk, "pwm6_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM6_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm7_bus_clk, "pwm7_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM7_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm8_bus_clk, "pwm8_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM8_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm9_bus_clk, "pwm9_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM9_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm10_bus_clk, "pwm10_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM10_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm11_bus_clk, "pwm11_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM11_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm12_bus_clk, "pwm12_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM12_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm13_bus_clk, "pwm13_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM13_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm14_bus_clk, "pwm14_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM14_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm15_bus_clk, "pwm15_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM15_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm16_bus_clk, "pwm16_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM16_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm17_bus_clk, "pwm17_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM17_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm18_bus_clk, "pwm18_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM18_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(pwm19_bus_clk, "pwm19_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_PWM19_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(ssp3_bus_clk, "ssp3_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_SSP3_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(rtc_bus_clk, "rtc_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_RTC_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(twsi0_bus_clk, "twsi0_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI0_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi1_bus_clk, "twsi1_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI1_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi2_bus_clk, "twsi2_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI2_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi4_bus_clk, "twsi4_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI4_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi5_bus_clk, "twsi5_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI5_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi6_bus_clk, "twsi6_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI6_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi7_bus_clk, "twsi7_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI7_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(twsi8_bus_clk, "twsi8_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TWSI8_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(timers1_bus_clk, "timers1_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TIMERS1_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(timers2_bus_clk, "timers2_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TIMERS2_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(aib_bus_clk, "aib_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_AIB_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(onewire_bus_clk, "onewire_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_ONEWIRE_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(sspa0_bus_clk, "sspa0_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_SSPA0_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(sspa1_bus_clk, "sspa1_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_SSPA1_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(tsen_bus_clk, "tsen_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_TSEN_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);

static CCU_GATE_DEFINE(ipc_ap2aud_bus_clk, "ipc_ap2aud_bus_clk", CCU_PARENT_HW(apb_clk),
		       APBC_IPC_AP2AUD_CLK_RST,
		       BIT(0), BIT(0), 0x0,
		       0);
/*	APBC clocks end		*/

/*	APMU clocks start	*/
static const struct clk_parent_data pmua_aclk_parents[] = {
	CCU_PARENT_HW(pll1_d10_245p76),
	CCU_PARENT_HW(pll1_d8_307p2),
};
static CCU_DIV_FC_MUX_DEFINE(pmua_aclk, "pmua_aclk", pmua_aclk_parents,
			     APMU_ACLK_CLK_CTRL,
			     1, 2, BIT(4),
			     0, 1,
			     0);

static const struct clk_parent_data cci550_clk_parents[] = {
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll2_d3),
};
static CCU_DIV_FC_MUX_DEFINE(cci550_clk, "cci550_clk", cci550_clk_parents,
			     APMU_CCI550_CLK_CTRL,
			     8, 3, BIT(12), 0, 2, CLK_IS_CRITICAL);

static const struct clk_parent_data cpu_c0_hi_clk_parents[] = {
	CCU_PARENT_HW(pll3_d2),
	CCU_PARENT_HW(pll3_d1),
};
static CCU_MUX_DEFINE(cpu_c0_hi_clk, "cpu_c0_hi_clk", cpu_c0_hi_clk_parents,
		      APMU_CPU_C0_CLK_CTRL,
		      13, 1, 0);
static const struct clk_parent_data cpu_c0_clk_parents[] = {
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d2_1228p8),
	CCU_PARENT_HW(pll3_d3),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(cpu_c0_hi_clk),
};
static CCU_MUX_FC_DEFINE(cpu_c0_core_clk, "cpu_c0_core_clk", cpu_c0_clk_parents,
			 APMU_CPU_C0_CLK_CTRL,
			 BIT(12), 0, 3, CLK_IS_CRITICAL);
static CCU_DIV_DEFINE(cpu_c0_ace_clk, "cpu_c0_ace_clk", CCU_PARENT_HW(cpu_c0_core_clk),
		      APMU_CPU_C0_CLK_CTRL,
		      6, 3, CLK_IS_CRITICAL);
static CCU_DIV_DEFINE(cpu_c0_tcm_clk, "cpu_c0_tcm_clk", CCU_PARENT_HW(cpu_c0_core_clk),
		      APMU_CPU_C0_CLK_CTRL, 9, 3, CLK_IS_CRITICAL);

static const struct clk_parent_data cpu_c1_hi_clk_parents[] = {
	CCU_PARENT_HW(pll3_d2),
	CCU_PARENT_HW(pll3_d1),
};
static CCU_MUX_DEFINE(cpu_c1_hi_clk, "cpu_c1_hi_clk", cpu_c1_hi_clk_parents,
		      APMU_CPU_C1_CLK_CTRL,
		      13, 1, CLK_IS_CRITICAL);
static const struct clk_parent_data cpu_c1_clk_parents[] = {
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d2_1228p8),
	CCU_PARENT_HW(pll3_d3),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(cpu_c1_hi_clk),
};
static CCU_MUX_FC_DEFINE(cpu_c1_core_clk, "cpu_c1_core_clk", cpu_c1_clk_parents,
			 APMU_CPU_C1_CLK_CTRL,
			 BIT(12), 0, 3, CLK_IS_CRITICAL);
static CCU_DIV_DEFINE(cpu_c1_ace_clk, "cpu_c1_ace_clk", CCU_PARENT_HW(cpu_c1_core_clk),
		      APMU_CPU_C1_CLK_CTRL,
		      6, 3, CLK_IS_CRITICAL);

static const struct clk_parent_data jpg_parents[] = {
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d2_1228p8),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll2_d3),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(jpg_clk, "jpg_clk", jpg_parents,
				  APMU_JPG_CLK_RES_CTRL,
				  5, 3, BIT(15),
				  2, 3, BIT(1), BIT(1), 0x0,
				  0);

static const struct clk_parent_data ccic2phy_parents[] = {
	CCU_PARENT_HW(pll1_d24_102p4),
	CCU_PARENT_HW(pll1_d48_51p2_ap),
};
static CCU_MUX_GATE_DEFINE(ccic2phy_clk, "ccic2phy_clk", ccic2phy_parents,
			   APMU_CSI_CCIC2_CLK_RES_CTRL,
			   7, 1, BIT(5), BIT(5), 0x0,
			   0);

static const struct clk_parent_data ccic3phy_parents[] = {
	CCU_PARENT_HW(pll1_d24_102p4),
	CCU_PARENT_HW(pll1_d48_51p2_ap),
};
static CCU_MUX_GATE_DEFINE(ccic3phy_clk, "ccic3phy_clk", ccic3phy_parents,
			   APMU_CSI_CCIC2_CLK_RES_CTRL,
			   31, 1, BIT(30), BIT(30), 0x0,
			   0);

static const struct clk_parent_data csi_parents[] = {
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll2_d2),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll1_d2_1228p8),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(csi_clk, "csi_clk", csi_parents,
				  APMU_CSI_CCIC2_CLK_RES_CTRL,
				  20, 3, BIT(15),
				  16, 3, BIT(4), BIT(4), 0x0,
				  0);

static const struct clk_parent_data camm_parents[] = {
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll2_d5),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_NAME(vctcxo_24m),
};
static CCU_DIV_MUX_GATE_DEFINE(camm0_clk, "camm0_clk", camm_parents,
			       APMU_CSI_CCIC2_CLK_RES_CTRL,
			       23, 4, 8, 2,
			       BIT(28), BIT(28), 0x0,
			       0);
static CCU_DIV_MUX_GATE_DEFINE(camm1_clk, "camm1_clk", camm_parents,
			       APMU_CSI_CCIC2_CLK_RES_CTRL,
			       23, 4, 8, 2, BIT(6), BIT(6), 0x0,
			       0);
static CCU_DIV_MUX_GATE_DEFINE(camm2_clk, "camm2_clk", camm_parents,
			       APMU_CSI_CCIC2_CLK_RES_CTRL,
			       23, 4, 8, 2, BIT(3), BIT(3), 0x0,
			       0);

static const struct clk_parent_data isp_cpp_parents[] = {
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d6_409p6),
};
static CCU_DIV_MUX_GATE_DEFINE(isp_cpp_clk, "isp_cpp_clk", isp_cpp_parents,
			       APMU_ISP_CLK_RES_CTRL,
			       24, 2, 26, 1, BIT(28), BIT(28), 0x0,
			       0);
static const struct clk_parent_data isp_bus_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d10_245p76),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(isp_bus_clk, "isp_bus_clk", isp_bus_parents,
				  APMU_ISP_CLK_RES_CTRL,
				  18, 3, BIT(23),
				  21, 2, BIT(17), BIT(17), 0x0,
				  0);
static const struct clk_parent_data isp_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d8_307p2),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(isp_clk, "isp_clk", isp_parents,
				  APMU_ISP_CLK_RES_CTRL,
				  4, 3, BIT(7),
				  8, 2, BIT(1), BIT(1), 0x0,
				  0);

static const struct clk_parent_data dpumclk_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d8_307p2),
};
static CCU_DIV2_FC_MUX_GATE_DEFINE(dpu_mclk, "dpu_mclk", dpumclk_parents,
				   APMU_LCD_CLK_RES_CTRL2,
				   APMU_LCD_CLK_RES_CTRL1,
				   1, 4, BIT(29),
				   5, 3, BIT(0), BIT(0), 0x0,
				   0);

static const struct clk_parent_data dpuesc_parents[] = {
	CCU_PARENT_HW(pll1_d48_51p2_ap),
	CCU_PARENT_HW(pll1_d52_47p26),
	CCU_PARENT_HW(pll1_d96_25p6),
	CCU_PARENT_HW(pll1_d32_76p8),
};
static CCU_MUX_GATE_DEFINE(dpu_esc_clk, "dpu_esc_clk", dpuesc_parents,
			   APMU_LCD_CLK_RES_CTRL1,
			   0, 2, BIT(2), BIT(2), 0x0,
			   0);

static const struct clk_parent_data dpubit_parents[] = {
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll2_d2),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll1_d2_1228p8),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll2_d5),
	CCU_PARENT_HW(pll2_d8),
	CCU_PARENT_HW(pll2_d8),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(dpu_bit_clk, "dpu_bit_clk", dpubit_parents,
				  APMU_LCD_CLK_RES_CTRL1,
				  17, 3, BIT(31),
				  20, 3, BIT(16), BIT(16), 0x0,
				  0);

static const struct clk_parent_data dpupx_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll2_d7),
	CCU_PARENT_HW(pll2_d8),
};
static CCU_DIV2_FC_MUX_GATE_DEFINE(dpu_pxclk, "dpu_pxclk", dpupx_parents,
				  APMU_LCD_CLK_RES_CTRL2,
				  APMU_LCD_CLK_RES_CTRL1,
				  17, 4, BIT(30),
				  21, 3, BIT(16), BIT(16), 0x0,
				  0);

static CCU_GATE_DEFINE(dpu_hclk, "dpu_hclk", CCU_PARENT_HW(pmua_aclk),
		       APMU_LCD_CLK_RES_CTRL1,
		       BIT(5), BIT(5), 0x0,
		       0);

static const struct clk_parent_data dpu_spi_parents[] = {
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d10_245p76),
	CCU_PARENT_HW(pll1_d11_223p4),
	CCU_PARENT_HW(pll1_d13_189),
	CCU_PARENT_HW(pll1_d23_106p8),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll2_d5),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(dpu_spi_clk, "dpu_spi_clk", dpu_spi_parents,
				  APMU_LCD_SPI_CLK_RES_CTRL,
				  8, 3, BIT(7),
				  12, 3, BIT(1), BIT(1), 0x0,
				  0);
static CCU_GATE_DEFINE(dpu_spi_hbus_clk, "dpu_spi_hbus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_LCD_SPI_CLK_RES_CTRL,
		       BIT(3), BIT(3), 0x0,
		       0);
static CCU_GATE_DEFINE(dpu_spi_bus_clk, "dpu_spi_bus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_LCD_SPI_CLK_RES_CTRL,
		       BIT(5), BIT(5), 0x0,
		       0);
static CCU_GATE_DEFINE(dpu_spi_aclk, "dpu_spi_aclk", CCU_PARENT_HW(pmua_aclk),
		       APMU_LCD_SPI_CLK_RES_CTRL,
		       BIT(6), BIT(6), 0x0,
		       0);

static const struct clk_parent_data v2d_parents[] = {
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d4_614p4),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(v2d_clk, "v2d_clk", v2d_parents,
				  APMU_LCD_CLK_RES_CTRL1,
				  9, 3, BIT(28),
				  12, 2, BIT(8), BIT(8), 0x0,
				  0);

static const struct clk_parent_data ccic_4x_parents[] = {
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll2_d2),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll1_d2_1228p8),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(ccic_4x_clk, "ccic_4x_clk", ccic_4x_parents,
				  APMU_CCIC_CLK_RES_CTRL,
				  18, 3, BIT(15),
				  23, 2, BIT(4), BIT(4), 0x0,
				  0);

static const struct clk_parent_data ccic1phy_parents[] = {
	CCU_PARENT_HW(pll1_d24_102p4),
	CCU_PARENT_HW(pll1_d48_51p2_ap),
};
static CCU_MUX_GATE_DEFINE(ccic1phy_clk, "ccic1phy_clk", ccic1phy_parents,
			   APMU_CCIC_CLK_RES_CTRL,
			   7, 1, BIT(5), BIT(5), 0x0,
			   0);

static CCU_GATE_DEFINE(sdh_axi_aclk, "sdh_axi_aclk", CCU_PARENT_HW(pmua_aclk),
		       APMU_SDH0_CLK_RES_CTRL,
		       BIT(3), BIT(3), 0x0,
		       0);
static const struct clk_parent_data sdh01_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll2_d8),
	CCU_PARENT_HW(pll2_d5),
	CCU_PARENT_HW(pll1_d11_223p4),
	CCU_PARENT_HW(pll1_d13_189),
	CCU_PARENT_HW(pll1_d23_106p8),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(sdh0_clk, "sdh0_clk", sdh01_parents,
				  APMU_SDH0_CLK_RES_CTRL,
				  8, 3, BIT(11),
				  5, 3, BIT(4), BIT(4), 0x0,
				  0);
static CCU_DIV_FC_MUX_GATE_DEFINE(sdh1_clk, "sdh1_clk", sdh01_parents,
				  APMU_SDH1_CLK_RES_CTRL,
				  8, 3, BIT(11),
				  5, 3, BIT(4), BIT(4), 0x0,
				  0);
static const struct clk_parent_data sdh2_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll2_d8),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d11_223p4),
	CCU_PARENT_HW(pll1_d13_189),
	CCU_PARENT_HW(pll1_d23_106p8),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(sdh2_clk, "sdh2_clk", sdh2_parents,
				  APMU_SDH2_CLK_RES_CTRL,
				  8, 3, BIT(11),
				  5, 3, BIT(4), BIT(4), 0x0,
				  0);

static CCU_GATE_DEFINE(usb_axi_clk, "usb_axi_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_USB_CLK_RES_CTRL,
		       BIT(1), BIT(1), 0x0,
		       0);
static CCU_GATE_DEFINE(usb_p1_aclk, "usb_p1_aclk", CCU_PARENT_HW(pmua_aclk),
		       APMU_USB_CLK_RES_CTRL,
		       BIT(5), BIT(5), 0x0,
		       0);
static CCU_GATE_DEFINE(usb30_clk, "usb30_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_USB_CLK_RES_CTRL,
		       BIT(8), BIT(8), 0x0,
		       0);

static const struct clk_parent_data qspi_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll2_d8),
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d10_245p76),
	CCU_PARENT_HW(pll1_d11_223p4),
	CCU_PARENT_HW(pll1_d23_106p8),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d13_189),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(qspi_clk, "qspi_clk", qspi_parents,
				   APMU_QSPI_CLK_RES_CTRL,
				   9, 3, BIT(12),
				   6, 3, BIT(4), BIT(4), 0x0,
				   0);
static CCU_GATE_DEFINE(qspi_bus_clk, "qspi_bus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_QSPI_CLK_RES_CTRL,
		       BIT(3), BIT(3), 0x0,
		       0);
static CCU_GATE_DEFINE(dma_clk, "dma_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_DMA_CLK_RES_CTRL,
		       BIT(3), BIT(3), 0x0,
		       0);

static const struct clk_parent_data aes_parents[] = {
	CCU_PARENT_HW(pll1_d12_204p8),
	CCU_PARENT_HW(pll1_d24_102p4),
};
static CCU_MUX_GATE_DEFINE(aes_clk, "aes_clk", aes_parents,
			   APMU_AES_CLK_RES_CTRL,
			   6, 1, BIT(5), BIT(5), 0x0,
			   0);

static const struct clk_parent_data vpu_parents[] = {
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll3_d6),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll2_d5),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(vpu_clk, "vpu_clk", vpu_parents,
				  APMU_VPU_CLK_RES_CTRL,
				  13, 3, BIT(21),
				  10, 3,
				  BIT(3), BIT(3), 0x0,
				  0);

static const struct clk_parent_data gpu_parents[] = {
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d3_819p2),
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll3_d6),
	CCU_PARENT_HW(pll2_d3),
	CCU_PARENT_HW(pll2_d4),
	CCU_PARENT_HW(pll2_d5),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(gpu_clk, "gpu_clk", gpu_parents,
				  APMU_GPU_CLK_RES_CTRL,
				  12, 3, BIT(15),
				  18, 3,
				  BIT(4), BIT(4), 0x0,
				  0);

static const struct clk_parent_data emmc_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d52_47p26),
	CCU_PARENT_HW(pll1_d3_819p2),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(emmc_clk, "emmc_clk", emmc_parents,
				  APMU_PMUA_EM_CLK_RES_CTRL,
				  8, 3, BIT(11),
				  6, 2,
				  BIT(4), BIT(4), 0x0,
				  0);
static CCU_DIV_GATE_DEFINE(emmc_x_clk, "emmc_x_clk", CCU_PARENT_HW(pll1_d2_1228p8),
			   APMU_PMUA_EM_CLK_RES_CTRL,
			   12, 3, BIT(15), BIT(15), 0x0,
			   0);

static const struct clk_parent_data audio_parents[] = {
	CCU_PARENT_HW(pll1_aud_245p7),
	CCU_PARENT_HW(pll1_d8_307p2),
	CCU_PARENT_HW(pll1_d6_409p6),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(audio_clk, "audio_clk", audio_parents,
				  APMU_AUDIO_CLK_RES_CTRL,
				  4, 3, BIT(15),
				  7, 3,
				  BIT(12), BIT(12), 0x0,
				  0);

static const struct clk_parent_data hdmi_parents[] = {
	CCU_PARENT_HW(pll1_d6_409p6),
	CCU_PARENT_HW(pll1_d5_491p52),
	CCU_PARENT_HW(pll1_d4_614p4),
	CCU_PARENT_HW(pll1_d8_307p2),
};
static CCU_DIV_FC_MUX_GATE_DEFINE(hdmi_mclk, "hdmi_mclk", hdmi_parents,
				  APMU_HDMI_CLK_RES_CTRL,
				  1, 4, BIT(29),
				  5, 3,
				  BIT(0), BIT(0), 0x0,
				  0);

static CCU_GATE_DEFINE(pcie0_clk, "pcie0_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_PCIE_CLK_RES_CTRL_0,
		       0x7, 0x7, 0x0,
		       0);
static CCU_GATE_DEFINE(pcie1_clk, "pcie1_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_PCIE_CLK_RES_CTRL_1,
		       0x7, 0x7, 0x0,
		       0);
static CCU_GATE_DEFINE(pcie2_clk, "pcie2_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_PCIE_CLK_RES_CTRL_2,
		       0x7, 0x7, 0x0,
		       0);

static CCU_GATE_DEFINE(emac0_bus_clk, "emac0_bus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_EMAC0_CLK_RES_CTRL,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(emac0_ptp_clk, "emac0_ptp_clk", CCU_PARENT_HW(pll2_d6),
		       APMU_EMAC0_CLK_RES_CTRL,
		       BIT(15), BIT(15), 0x0,
		       0);
static CCU_GATE_DEFINE(emac1_bus_clk, "emac1_bus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_EMAC1_CLK_RES_CTRL,
		       BIT(0), BIT(0), 0x0,
		       0);
static CCU_GATE_DEFINE(emac1_ptp_clk, "emac1_ptp_clk", CCU_PARENT_HW(pll2_d6),
		       APMU_EMAC1_CLK_RES_CTRL,
		       BIT(15), BIT(15), 0x0,
		       0);

static CCU_GATE_DEFINE(emmc_bus_clk, "emmc_bus_clk", CCU_PARENT_HW(pmua_aclk),
		       APMU_PMUA_EM_CLK_RES_CTRL,
		       BIT(3), BIT(3), 0,
		       0);
/*	APMU clocks end		*/

static struct clk_hw_onecell_data k1_ccu_apbs_clks = {
	.hws = {
		[CLK_PLL1]		= &pll1.common.hw,
		[CLK_PLL2]		= &pll2.common.hw,
		[CLK_PLL3]		= &pll3.common.hw,
		[CLK_PLL1_D2]		= &pll1_d2.common.hw,
		[CLK_PLL1_D3]		= &pll1_d3.common.hw,
		[CLK_PLL1_D4]		= &pll1_d4.common.hw,
		[CLK_PLL1_D5]		= &pll1_d5.common.hw,
		[CLK_PLL1_D6]		= &pll1_d6.common.hw,
		[CLK_PLL1_D7]		= &pll1_d7.common.hw,
		[CLK_PLL1_D8]		= &pll1_d8.common.hw,
		[CLK_PLL1_D11]		= &pll1_d11_223p4.common.hw,
		[CLK_PLL1_D13]		= &pll1_d13_189.common.hw,
		[CLK_PLL1_D23]		= &pll1_d23_106p8.common.hw,
		[CLK_PLL1_D64]		= &pll1_d64_38p4.common.hw,
		[CLK_PLL1_D10_AUD]	= &pll1_aud_245p7.common.hw,
		[CLK_PLL1_D100_AUD]	= &pll1_aud_24p5.common.hw,
		[CLK_PLL2_D1]		= &pll2_d1.common.hw,
		[CLK_PLL2_D2]		= &pll2_d2.common.hw,
		[CLK_PLL2_D3]		= &pll2_d3.common.hw,
		[CLK_PLL2_D4]		= &pll2_d4.common.hw,
		[CLK_PLL2_D5]		= &pll2_d5.common.hw,
		[CLK_PLL2_D6]		= &pll2_d6.common.hw,
		[CLK_PLL2_D7]		= &pll2_d7.common.hw,
		[CLK_PLL2_D8]		= &pll2_d8.common.hw,
		[CLK_PLL3_D1]		= &pll3_d1.common.hw,
		[CLK_PLL3_D2]		= &pll3_d2.common.hw,
		[CLK_PLL3_D3]		= &pll3_d3.common.hw,
		[CLK_PLL3_D4]		= &pll3_d4.common.hw,
		[CLK_PLL3_D5]		= &pll3_d5.common.hw,
		[CLK_PLL3_D6]		= &pll3_d6.common.hw,
		[CLK_PLL3_D7]		= &pll3_d7.common.hw,
		[CLK_PLL3_D8]		= &pll3_d8.common.hw,
		[CLK_PLL3_80]		= &pll3_80.common.hw,
		[CLK_PLL3_40]		= &pll3_40.common.hw,
		[CLK_PLL3_20]		= &pll3_20.common.hw,

	},
	.num = CLK_APBS_NUM,
};

static struct clk_hw_onecell_data k1_ccu_mpmu_clks = {
	.hws = {
		[CLK_PLL1_307P2]	= &pll1_d8_307p2.common.hw,
		[CLK_PLL1_76P8]		= &pll1_d32_76p8.common.hw,
		[CLK_PLL1_61P44]	= &pll1_d40_61p44.common.hw,
		[CLK_PLL1_153P6]	= &pll1_d16_153p6.common.hw,
		[CLK_PLL1_102P4]	= &pll1_d24_102p4.common.hw,
		[CLK_PLL1_51P2]		= &pll1_d48_51p2.common.hw,
		[CLK_PLL1_51P2_AP]	= &pll1_d48_51p2_ap.common.hw,
		[CLK_PLL1_57P6]		= &pll1_m3d128_57p6.common.hw,
		[CLK_PLL1_25P6]		= &pll1_d96_25p6.common.hw,
		[CLK_PLL1_12P8]		= &pll1_d192_12p8.common.hw,
		[CLK_PLL1_12P8_WDT]	= &pll1_d192_12p8_wdt.common.hw,
		[CLK_PLL1_6P4]		= &pll1_d384_6p4.common.hw,
		[CLK_PLL1_3P2]		= &pll1_d768_3p2.common.hw,
		[CLK_PLL1_1P6]		= &pll1_d1536_1p6.common.hw,
		[CLK_PLL1_0P8]		= &pll1_d3072_0p8.common.hw,
		[CLK_PLL1_351]		= &pll1_d7_351p08.common.hw,
		[CLK_PLL1_409P6]	= &pll1_d6_409p6.common.hw,
		[CLK_PLL1_204P8]	= &pll1_d12_204p8.common.hw,
		[CLK_PLL1_491]		= &pll1_d5_491p52.common.hw,
		[CLK_PLL1_245P76]	= &pll1_d10_245p76.common.hw,
		[CLK_PLL1_614]		= &pll1_d4_614p4.common.hw,
		[CLK_PLL1_47P26]	= &pll1_d52_47p26.common.hw,
		[CLK_PLL1_31P5]		= &pll1_d78_31p5.common.hw,
		[CLK_PLL1_819]		= &pll1_d3_819p2.common.hw,
		[CLK_PLL1_1228]		= &pll1_d2_1228p8.common.hw,
		[CLK_SLOW_UART]		= &slow_uart.common.hw,
		[CLK_SLOW_UART1]	= &slow_uart1_14p74.common.hw,
		[CLK_SLOW_UART2]	= &slow_uart2_48.common.hw,
		[CLK_WDT]		= &wdt_clk.common.hw,
		[CLK_RIPC]		= &ripc_clk.common.hw,
		[CLK_I2S_SYSCLK]	= &i2s_sysclk.common.hw,
		[CLK_I2S_BCLK]		= &i2s_bclk.common.hw,
		[CLK_APB]		= &apb_clk.common.hw,
		[CLK_WDT_BUS]		= &wdt_bus_clk.common.hw,
	},
	.num = CLK_MPMU_NUM,
};

static struct clk_hw_onecell_data k1_ccu_apbc_clks = {
	.hws = {
		[CLK_UART0]		= &uart0_clk.common.hw,
		[CLK_UART2]		= &uart2_clk.common.hw,
		[CLK_UART3]		= &uart3_clk.common.hw,
		[CLK_UART4]		= &uart4_clk.common.hw,
		[CLK_UART5]		= &uart5_clk.common.hw,
		[CLK_UART6]		= &uart6_clk.common.hw,
		[CLK_UART7]		= &uart7_clk.common.hw,
		[CLK_UART8]		= &uart8_clk.common.hw,
		[CLK_UART9]		= &uart9_clk.common.hw,
		[CLK_GPIO]		= &gpio_clk.common.hw,
		[CLK_PWM0]		= &pwm0_clk.common.hw,
		[CLK_PWM1]		= &pwm1_clk.common.hw,
		[CLK_PWM2]		= &pwm2_clk.common.hw,
		[CLK_PWM3]		= &pwm3_clk.common.hw,
		[CLK_PWM4]		= &pwm4_clk.common.hw,
		[CLK_PWM5]		= &pwm5_clk.common.hw,
		[CLK_PWM6]		= &pwm6_clk.common.hw,
		[CLK_PWM7]		= &pwm7_clk.common.hw,
		[CLK_PWM8]		= &pwm8_clk.common.hw,
		[CLK_PWM9]		= &pwm9_clk.common.hw,
		[CLK_PWM10]		= &pwm10_clk.common.hw,
		[CLK_PWM11]		= &pwm11_clk.common.hw,
		[CLK_PWM12]		= &pwm12_clk.common.hw,
		[CLK_PWM13]		= &pwm13_clk.common.hw,
		[CLK_PWM14]		= &pwm14_clk.common.hw,
		[CLK_PWM15]		= &pwm15_clk.common.hw,
		[CLK_PWM16]		= &pwm16_clk.common.hw,
		[CLK_PWM17]		= &pwm17_clk.common.hw,
		[CLK_PWM18]		= &pwm18_clk.common.hw,
		[CLK_PWM19]		= &pwm19_clk.common.hw,
		[CLK_SSP3]		= &ssp3_clk.common.hw,
		[CLK_RTC]		= &rtc_clk.common.hw,
		[CLK_TWSI0]		= &twsi0_clk.common.hw,
		[CLK_TWSI1]		= &twsi1_clk.common.hw,
		[CLK_TWSI2]		= &twsi2_clk.common.hw,
		[CLK_TWSI4]		= &twsi4_clk.common.hw,
		[CLK_TWSI5]		= &twsi5_clk.common.hw,
		[CLK_TWSI6]		= &twsi6_clk.common.hw,
		[CLK_TWSI7]		= &twsi7_clk.common.hw,
		[CLK_TWSI8]		= &twsi8_clk.common.hw,
		[CLK_TIMERS1]		= &timers1_clk.common.hw,
		[CLK_TIMERS2]		= &timers2_clk.common.hw,
		[CLK_AIB]		= &aib_clk.common.hw,
		[CLK_ONEWIRE]		= &onewire_clk.common.hw,
		[CLK_SSPA0]		= &sspa0_clk.common.hw,
		[CLK_SSPA1]		= &sspa1_clk.common.hw,
		[CLK_DRO]		= &dro_clk.common.hw,
		[CLK_IR]		= &ir_clk.common.hw,
		[CLK_TSEN]		= &tsen_clk.common.hw,
		[CLK_IPC_AP2AUD]	= &ipc_ap2aud_clk.common.hw,
		[CLK_CAN0]		= &can0_clk.common.hw,
		[CLK_CAN0_BUS]		= &can0_bus_clk.common.hw,
		[CLK_UART0_BUS]		= &uart0_bus_clk.common.hw,
		[CLK_UART2_BUS]		= &uart2_bus_clk.common.hw,
		[CLK_UART3_BUS]		= &uart3_bus_clk.common.hw,
		[CLK_UART4_BUS]		= &uart4_bus_clk.common.hw,
		[CLK_UART5_BUS]		= &uart5_bus_clk.common.hw,
		[CLK_UART6_BUS]		= &uart6_bus_clk.common.hw,
		[CLK_UART7_BUS]		= &uart7_bus_clk.common.hw,
		[CLK_UART8_BUS]		= &uart8_bus_clk.common.hw,
		[CLK_UART9_BUS]		= &uart9_bus_clk.common.hw,
		[CLK_GPIO_BUS]		= &gpio_bus_clk.common.hw,
		[CLK_PWM0_BUS]		= &pwm0_bus_clk.common.hw,
		[CLK_PWM1_BUS]		= &pwm1_bus_clk.common.hw,
		[CLK_PWM2_BUS]		= &pwm2_bus_clk.common.hw,
		[CLK_PWM3_BUS]		= &pwm3_bus_clk.common.hw,
		[CLK_PWM4_BUS]		= &pwm4_bus_clk.common.hw,
		[CLK_PWM5_BUS]		= &pwm5_bus_clk.common.hw,
		[CLK_PWM6_BUS]		= &pwm6_bus_clk.common.hw,
		[CLK_PWM7_BUS]		= &pwm7_bus_clk.common.hw,
		[CLK_PWM8_BUS]		= &pwm8_bus_clk.common.hw,
		[CLK_PWM9_BUS]		= &pwm9_bus_clk.common.hw,
		[CLK_PWM10_BUS]		= &pwm10_bus_clk.common.hw,
		[CLK_PWM11_BUS]		= &pwm11_bus_clk.common.hw,
		[CLK_PWM12_BUS]		= &pwm12_bus_clk.common.hw,
		[CLK_PWM13_BUS]		= &pwm13_bus_clk.common.hw,
		[CLK_PWM14_BUS]		= &pwm14_bus_clk.common.hw,
		[CLK_PWM15_BUS]		= &pwm15_bus_clk.common.hw,
		[CLK_PWM16_BUS]		= &pwm16_bus_clk.common.hw,
		[CLK_PWM17_BUS]		= &pwm17_bus_clk.common.hw,
		[CLK_PWM18_BUS]		= &pwm18_bus_clk.common.hw,
		[CLK_PWM19_BUS]		= &pwm19_bus_clk.common.hw,
		[CLK_SSP3_BUS]		= &ssp3_bus_clk.common.hw,
		[CLK_RTC_BUS]		= &rtc_bus_clk.common.hw,
		[CLK_TWSI0_BUS]		= &twsi0_bus_clk.common.hw,
		[CLK_TWSI1_BUS]		= &twsi1_bus_clk.common.hw,
		[CLK_TWSI2_BUS]		= &twsi2_bus_clk.common.hw,
		[CLK_TWSI4_BUS]		= &twsi4_bus_clk.common.hw,
		[CLK_TWSI5_BUS]		= &twsi5_bus_clk.common.hw,
		[CLK_TWSI6_BUS]		= &twsi6_bus_clk.common.hw,
		[CLK_TWSI7_BUS]		= &twsi7_bus_clk.common.hw,
		[CLK_TWSI8_BUS]		= &twsi8_bus_clk.common.hw,
		[CLK_TIMERS1_BUS]	= &timers1_bus_clk.common.hw,
		[CLK_TIMERS2_BUS]	= &timers2_bus_clk.common.hw,
		[CLK_AIB_BUS]		= &aib_bus_clk.common.hw,
		[CLK_ONEWIRE_BUS]	= &onewire_bus_clk.common.hw,
		[CLK_SSPA0_BUS]		= &sspa0_bus_clk.common.hw,
		[CLK_SSPA1_BUS]		= &sspa1_bus_clk.common.hw,
		[CLK_TSEN_BUS]		= &tsen_bus_clk.common.hw,
		[CLK_IPC_AP2AUD_BUS]	= &ipc_ap2aud_bus_clk.common.hw,
	},
	.num = CLK_APBC_NUM,
};

static struct clk_hw_onecell_data k1_ccu_apmu_clks = {
	.hws = {
		[CLK_CCI550]		= &cci550_clk.common.hw,
		[CLK_CPU_C0_HI]		= &cpu_c0_hi_clk.common.hw,
		[CLK_CPU_C0_CORE]	= &cpu_c0_core_clk.common.hw,
		[CLK_CPU_C0_ACE]	= &cpu_c0_ace_clk.common.hw,
		[CLK_CPU_C0_TCM]	= &cpu_c0_tcm_clk.common.hw,
		[CLK_CPU_C1_HI]		= &cpu_c1_hi_clk.common.hw,
		[CLK_CPU_C1_CORE]	= &cpu_c1_core_clk.common.hw,
		[CLK_CPU_C1_ACE]	= &cpu_c1_ace_clk.common.hw,
		[CLK_CCIC_4X]		= &ccic_4x_clk.common.hw,
		[CLK_CCIC1PHY]		= &ccic1phy_clk.common.hw,
		[CLK_SDH_AXI]		= &sdh_axi_aclk.common.hw,
		[CLK_SDH0]		= &sdh0_clk.common.hw,
		[CLK_SDH1]		= &sdh1_clk.common.hw,
		[CLK_SDH2]		= &sdh2_clk.common.hw,
		[CLK_USB_P1]		= &usb_p1_aclk.common.hw,
		[CLK_USB_AXI]		= &usb_axi_clk.common.hw,
		[CLK_USB30]		= &usb30_clk.common.hw,
		[CLK_QSPI]		= &qspi_clk.common.hw,
		[CLK_QSPI_BUS]		= &qspi_bus_clk.common.hw,
		[CLK_DMA]		= &dma_clk.common.hw,
		[CLK_AES]		= &aes_clk.common.hw,
		[CLK_VPU]		= &vpu_clk.common.hw,
		[CLK_GPU]		= &gpu_clk.common.hw,
		[CLK_EMMC]		= &emmc_clk.common.hw,
		[CLK_EMMC_X]		= &emmc_x_clk.common.hw,
		[CLK_AUDIO]		= &audio_clk.common.hw,
		[CLK_HDMI]		= &hdmi_mclk.common.hw,
		[CLK_PMUA_ACLK]		= &pmua_aclk.common.hw,
		[CLK_PCIE0]		= &pcie0_clk.common.hw,
		[CLK_PCIE1]		= &pcie1_clk.common.hw,
		[CLK_PCIE2]		= &pcie2_clk.common.hw,
		[CLK_EMAC0_BUS]		= &emac0_bus_clk.common.hw,
		[CLK_EMAC0_PTP]		= &emac0_ptp_clk.common.hw,
		[CLK_EMAC1_BUS]		= &emac1_bus_clk.common.hw,
		[CLK_EMAC1_PTP]		= &emac1_ptp_clk.common.hw,
		[CLK_JPG]		= &jpg_clk.common.hw,
		[CLK_CCIC2PHY]		= &ccic2phy_clk.common.hw,
		[CLK_CCIC3PHY]		= &ccic3phy_clk.common.hw,
		[CLK_CSI]		= &csi_clk.common.hw,
		[CLK_CAMM0]		= &camm0_clk.common.hw,
		[CLK_CAMM1]		= &camm1_clk.common.hw,
		[CLK_CAMM2]		= &camm2_clk.common.hw,
		[CLK_ISP_CPP]		= &isp_cpp_clk.common.hw,
		[CLK_ISP_BUS]		= &isp_bus_clk.common.hw,
		[CLK_ISP]		= &isp_clk.common.hw,
		[CLK_DPU_MCLK]		= &dpu_mclk.common.hw,
		[CLK_DPU_ESC]		= &dpu_esc_clk.common.hw,
		[CLK_DPU_BIT]		= &dpu_bit_clk.common.hw,
		[CLK_DPU_PXCLK]		= &dpu_pxclk.common.hw,
		[CLK_DPU_HCLK]		= &dpu_hclk.common.hw,
		[CLK_DPU_SPI]		= &dpu_spi_clk.common.hw,
		[CLK_DPU_SPI_HBUS]	= &dpu_spi_hbus_clk.common.hw,
		[CLK_DPU_SPIBUS]	= &dpu_spi_bus_clk.common.hw,
		[CLK_DPU_SPI_ACLK]	= &dpu_spi_aclk.common.hw,
		[CLK_V2D]		= &v2d_clk.common.hw,
		[CLK_EMMC_BUS]		= &emmc_bus_clk.common.hw,
	},
	.num = CLK_APMU_NUM
};

struct spacemit_ccu_data {
	struct clk_hw_onecell_data *hw_clks;
	bool need_pll_lock;
};

struct spacemit_ccu_priv {
	const struct spacemit_ccu_data *data;
	struct regmap *base;
	struct regmap *lock_base;
};

static int spacemit_ccu_register(struct device *dev,
				 struct spacemit_ccu_priv *priv)
{
	const struct spacemit_ccu_data *data = priv->data;
	int i, ret;

	for (i = 0; i < data->hw_clks->num; i++) {
		struct clk_hw *hw = data->hw_clks->hws[i];
		struct ccu_common *common;
		const char *name;

		if (!hw)
			continue;

		common = hw_to_ccu_common(hw);
		name = hw->init->name;

		common->base		= priv->base;
		common->lock_base	= priv->lock_base;

		ret = devm_clk_hw_register(dev, hw);
		if (ret) {
			dev_err(dev, "Cannot register clock %d - %s\n",
				i, name);
			return ret;
		}
	}

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   data->hw_clks);
}

static int k1_ccu_probe(struct platform_device *pdev)
{
	const struct spacemit_ccu_data *data;
	struct regmap *base_map, *lock_map = NULL;
	struct device *dev = &pdev->dev;
	struct spacemit_ccu_priv *priv;
	struct device_node *parent;
	int ret;

	data = of_device_get_match_data(dev);
	if (WARN_ON(!data))
		return -EINVAL;

	parent   = of_get_parent(dev->of_node);
	base_map = syscon_node_to_regmap(parent);
	of_node_put(parent);

	if (IS_ERR(base_map))
		return dev_err_probe(dev, PTR_ERR(base_map),
				     "failed to get regmap\n");

	if (data->need_pll_lock) {
		lock_map = syscon_regmap_lookup_by_phandle(dev->of_node,
							   "spacemit,mpmu");
		if (IS_ERR(lock_map))
			return dev_err_probe(dev, PTR_ERR(lock_map),
					     "failed to get lock regmap\n");
	}

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->data	= data;
	priv->base	= base_map;
	priv->lock_base	= lock_map;

	ret = spacemit_ccu_register(dev, priv);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register clocks\n");

	return 0;
}

static const struct spacemit_ccu_data k1_ccu_apbs_data = {
	.need_pll_lock	= true,
	.hw_clks	= &k1_ccu_apbs_clks,
};

static const struct spacemit_ccu_data k1_ccu_mpmu_data = {
	.need_pll_lock	= false,
	.hw_clks	= &k1_ccu_mpmu_clks,
};

static const struct spacemit_ccu_data k1_ccu_apbc_data = {
	.need_pll_lock	= false,
	.hw_clks	= &k1_ccu_apbc_clks,
};

static const struct spacemit_ccu_data k1_ccu_apmu_data = {
	.need_pll_lock	= false,
	.hw_clks	= &k1_ccu_apmu_clks,
};

static const struct of_device_id of_k1_ccu_match[] = {
	{
		.compatible	= "spacemit,k1-ccu-apbs",
		.data		= &k1_ccu_apbs_data,
	},
	{
		.compatible	= "spacemit,k1-ccu-mpmu",
		.data		= &k1_ccu_mpmu_data,
	},
	{
		.compatible	= "spacemit,k1-ccu-apbc",
		.data		= &k1_ccu_apbc_data,
	},
	{
		.compatible	= "spacemit,k1-ccu-apmu",
		.data		= &k1_ccu_apmu_data,
	},
	{ }
};
MODULE_DEVICE_TABLE(of, of_k1_ccu_match);

static struct platform_driver k1_ccu_driver = {
	.driver = {
		.name		= "spacemit,k1-ccu",
		.of_match_table = of_k1_ccu_match,
	},
	.probe	= k1_ccu_probe,
};
module_platform_driver(k1_ccu_driver);

MODULE_DESCRIPTION("Spacemit K1 CCU driver");
MODULE_AUTHOR("Haylen Chu <heylenay@4d2.org>");
MODULE_LICENSE("GPL");

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef __SPACEMIT_INNO_DP_H__
#define __SPACEMIT_INNO_DP_H__

/* Register field access uses GENMASK/BIT + FIELD_PREP/FIELD_GET. */
#include <linux/bits.h>

#define DP_CORE_CONTROL                      0x0018
#define  DP_SCRAMBLER_DISABLE               BIT(31)
#define  DP_ENHANCE_FRAMING_EN              BIT(30)
#define  DP_DEFAULT_FAST_LINK_TRAIN_EN      BIT(29)
#define  DP_SCALE_DOWN_MODE                 BIT(28)
#define  DP_FORCE_HPD                       BIT(27)
#define  DP_ENABLE_EDP                      BIT(4)

#define DP_SOFT_RESET                        0x001c
#define  DP_CONTROLLER_RESET                BIT(31)
#define  DP_PHY_RESET                       BIT(30)
#define  DP_HDCP_RESET                      BIT(29)
#define  DP_AUDIO_RESET                     BIT(28)
#define  DP_AUX_RESET                       BIT(27)
#define  DP_VIDEO_RESET                     GENMASK(3, 0)

#define DP_CLK_INV_MUX                       0x0028
#define  DP_REG_VID_CLK_SEL                 BIT(0)

/* Interrupt status latches; write 1 to clear. */
#define DP_GENERAL_INTERRUPT                 0x0080
#define  DP_AUX_REPLY_EVENT_INT_STA         BIT(16)

#define DP_GENERAL_INTERRUPT_MASK            0x0084
#define  DP_HPD_INT_STA_MSK                 BIT(17)
#define  DP_AUX_REPLY_EVENT_INT_STA_MSK     BIT(16)
#define  DP_HDCP_INT_STA_MSK                BIT(15)
#define  DP_ILLEGAL_AUX_CMD_INT_STA_MSK     BIT(14)
#define  DP_TYPE_C_EVENT_MSK                BIT(13)
#define  DP_DSC_EVENT_MSK                   BIT(12)
#define  DP_SDP_INT_STA_S3_MSK              BIT(11)
#define  DP_SDP_INT_STA_S2_MSK              BIT(10)
#define  DP_SDP_INT_STA_S1_MSK              BIT(9)
#define  DP_SDP_INT_STA_S0_MSK              BIT(8)
#define  DP_VIDEO_FIFO_OVERFLOW_INT_STA_S3_MSK BIT(7)
#define  DP_VIDEO_FIFO_OVERFLOW_INT_STA_S2_MSK BIT(6)
#define  DP_VIDEO_FIFO_OVERFLOW_INT_STA_S1_MSK BIT(5)
#define  DP_VIDEO_FIFO_OVERFLOW_INT_STA_S0_MSK BIT(4)

#define DP_HPD_INTERRUPT_STATUS              0x0088
#define  DP_HOT_PLUG_EVENT                  BIT(29)
#define  DP_HOT_UNPLUG_EVENT                BIT(28)
#define  DP_HPD_IN_STATUS                   BIT(26)

#define DP_HPD_INTERRUPT_ENABLE              0x008c
#define  DP_SINK_IRQ_EVENT_MSK              BIT(31)
#define  DP_HOT_PLUG_EVENT_MSK              BIT(29)
#define  DP_HOT_UNPLUG_EVENT_MSK            BIT(28)
#define  DP_SINK_UNPLUG_ERROR_EVENT_MSK     BIT(27)

#define DP_PHYIF_CTRL_ADDR                   0x0100
#define  DP_PHY_BUSY_BYP                    BIT(31)
#define  DP_TPS_SEL                         GENMASK(28, 25)
#define  DP_XMIT_ENABLE                     GENMASK(20, 17)
#define  DP_PHY_POWERDOWN                   GENMASK(16, 13)
#define  DP_PHY_SSC_DIS                     BIT(8)
#define  DP_PHY_NUM_LANES                   GENMASK(6, 5)
#define  DP_PHY_RATE                        GENMASK(1, 0)

#define DP_PHYIF_TX_EQ_ADDR                  0x0104
#define  DP_PHY_LANE3_TX_VSWING             GENMASK(21, 20)
#define  DP_PHY_LANE3_TX_PREEMP             GENMASK(19, 18)
#define  DP_PHY_LANE2_TX_VSWING             GENMASK(15, 14)
#define  DP_PHY_LANE2_TX_PREEMP             GENMASK(13, 12)
#define  DP_PHY_LANE1_TX_VSWING             GENMASK(9, 8)
#define  DP_PHY_LANE1_TX_PREEMP             GENMASK(7, 6)
#define  DP_PHY_LANE0_TX_VSWING             GENMASK(3, 2)
#define  DP_PHY_LANE0_TX_PREEMP             GENMASK(1, 0)

#define DP_MPLL_CTRL0                        0x0180
#define  DP_ANA_MPLL_FBDIV_LBIT             GENMASK(31, 24)
#define  DP_ANA_MPLL_SEL_EXTWAVE            BIT(22)
#define  DP_ANA_MPLL_DISABLE_SSCG           BIT(21)
#define  DP_ANA_MPLL_DOWNSPREAD             BIT(20)
#define  DP_ANA_MPLL_FBDIV_HBIT             GENMASK(19, 16)
#define  DP_ANA_MPLL_OBSEN                  BIT(15)
#define  DP_ANA_MPLL_OBSSEL                 BIT(14)
#define  DP_ANA_MPLL_PREDIV                 GENMASK(13, 8)
#define  DP_AD_LOCK_COREPLL                 BIT(7)
#define  DP_ANA_MPLL_DACPD                  BIT(5)
#define  DP_ANA_MPLL_DSMPD                  BIT(4)
#define  DP_ANA_MPLL_PD                     BIT(0)

#define DP_MPLL_CTRL1                        0x0184
#define  DP_ANA_MPLL_FRAC_LBIT              GENMASK(23, 16)
#define  DP_ANA_MPLL_FRAC_MBIT              GENMASK(15, 8)
#define  DP_ANA_MPLL_FRAC_HBIT              GENMASK(7, 0)

#define DP_MPLL_CTRL2                        0x0188
#define  DP_ANA_MPLL_VCOCLK_DIV8_EN         BIT(16)
#define  DP_ANA_MPLL_POSTDIVEN              BIT(11)
#define  DP_ANA_MPLL_POSTDIV                GENMASK(10, 8)

#define DP_PREPLL_CTRL0                      0x0190
#define  DP_ANA_PREPLL_FBDIV2_LBIT          GENMASK(31, 24)
#define  DP_ANA_PREPLL_FBDIV2_HBIT          GENMASK(19, 16)
#define  DP_ANA_PREPLL_PREDIV               GENMASK(13, 8)
#define  DP_AD_LOCK_PIXELPLL                BIT(7)
#define  DP_ANA_PREPLL_DACPD                BIT(5)
#define  DP_ANA_PREPLL_DSMPD                BIT(4)
#define  DP_REG_PCLK_OUTPUT_NORMAL          BIT(1)
#define  DP_ANA_PREPLL_PD                   BIT(0)

#define DP_PREPLL_CTRL1                      0x0194
#define  DP_ANA_PREPLL_PRECLK_DIVM          GENMASK(17, 16)
#define  DP_ANA_PREPLL_PRECLK_DIVAUX        GENMASK(12, 8)

#define DP_PREPLL_CTRL2                      0x0198
#define  DP_ANA_PREPLL_PCLKDIV5_EN          BIT(31)
#define  DP_ANA_PREPLL_PCLK_DIVAUX          GENMASK(28, 24)
#define  DP_ANA_PREPLL_LOCK_BYPEN           BIT(17)
#define  DP_ANA_PREPLL_LOWFRE_EN            BIT(16)
#define  DP_ANA_PREPLL_TESTEN               BIT(15)
#define  DP_ANA_PREPLL_TESTSEL              GENMASK(14, 12)
#define  DP_ANA_PREPLL_HDMI_EN              BIT(9)
#define  DP_ANA_PREPLL_DP_EN                BIT(8)

#define DP_PREPLL_SSC_CTRL                   0x019c
#define  DP_ANA_PREPLL_FRAC2_LBIT           GENMASK(23, 16)
#define  DP_ANA_PREPLL_FRAC2_MBIT           GENMASK(15, 8)
#define  DP_ANA_PREPLL_FRAC2_HBIT           GENMASK(7, 0)

#define DP_ANA_TX_CTRL0                      0x01a0
#define  DP_ANA_MPLL_CLKDIV_16M             GENMASK(13, 8)

#define DP_ANA_TX_ENABLE                     0x01a4
#define  DP_ANA_TX_ISEL_DRV_D3              GENMASK(31, 28)
#define  DP_ANA_TX_ISEL_DRV_D2              GENMASK(27, 24)
#define  DP_AD_TX_ESDSENSE_D3               GENMASK(23, 22)
#define  DP_AD_TX_ESDSENSE_D2               GENMASK(21, 20)
#define  DP_AD_TX_ESDSENSE_D1               GENMASK(19, 18)
#define  DP_AD_TX_ESDSENSE_D0               GENMASK(17, 16)
#define  DP_ANA_TX_EN_LDO_D3                BIT(11)
#define  DP_ANA_TX_EN_LDO_D2                BIT(10)
#define  DP_ANA_TX_EN_LDO_D1                BIT(9)
#define  DP_ANA_TX_EN_LDO_D0                BIT(8)
#define  DP_ANA_TX_EN_BYP_D3                BIT(7)
#define  DP_ANA_TX_EN_BYP_D2                BIT(6)
#define  DP_ANA_TX_EN_BYP_D1                BIT(5)
#define  DP_ANA_TX_EN_BYP_D0                BIT(4)
#define  DP_ANA_TX_EN_DRV_D3                BIT(3)
#define  DP_ANA_TX_EN_DRV_D2                BIT(2)
#define  DP_ANA_TX_EN_DRV_D1                BIT(1)
#define  DP_ANA_TX_EN_DRV_D0                BIT(0)

#define DP_ANA_TX_CTRL1                      0x01a8
#define  DP_ANA_TX_PHASE_D2                 GENMASK(31, 30)
#define  DP_ANA_TX_MAINSEL_D2               GENMASK(28, 24)
#define  DP_ANA_TX_PHASE_D3                 GENMASK(23, 22)
#define  DP_ANA_TX_MAINSEL_D3               GENMASK(20, 16)
#define  DP_ANA_TX_ISEL_LDO_D3              BIT(11)
#define  DP_ANA_TX_ISEL_LDO_D2              BIT(10)
#define  DP_ANA_TX_ISEL_LDO_D1              BIT(9)
#define  DP_ANA_TX_ISEL_LDO_D0              BIT(8)
#define  DP_ANA_TX_ISEL_DRV_D1              GENMASK(7, 4)
#define  DP_ANA_TX_ISEL_DRV_D0              GENMASK(3, 0)

#define DP_ANA_TX_CTRL2                      0x01ac
#define  DP_ANA_TX_POSTSEL_D1               GENMASK(31, 28)
#define  DP_ANA_TX_POSTSEL_D0               GENMASK(27, 24)
#define  DP_ANA_TX_POSTSEL_D3               GENMASK(23, 20)
#define  DP_ANA_TX_POSTSEL_D2               GENMASK(19, 16)
#define  DP_ANA_TX_PHASE_D0                 GENMASK(15, 14)
#define  DP_DA_TX_MAINSEL_D0_4_0            GENMASK(12, 8)
#define  DP_ANA_TX_PHASE_D1                 GENMASK(7, 6)
#define  DP_ANA_TX_MAINSEL_D1               GENMASK(4, 0)

#define DP_ANA_TX_CTRL3                      0x01b0
#define  DP_ANA_TX_EN_P2S_D3                BIT(31)
#define  DP_ANA_TX_EN_P2S_D2                BIT(30)
#define  DP_ANA_TX_EN_P2S_D1                BIT(29)
#define  DP_ANA_TX_EN_P2S_D0                BIT(28)
#define  DP_ANA_TX_MODE_D3                  BIT(27)
#define  DP_ANA_TX_MODE_D2                  BIT(26)
#define  DP_ANA_TX_MODE_D1                  BIT(25)
#define  DP_ANA_TX_MODE_D0                  BIT(24)
#define  DP_ANA_TX_PRESEL_D1                GENMASK(14, 12)
#define  DP_ANA_TX_PRESEL_D0                GENMASK(10, 8)
#define  DP_ANA_TX_PRESEL_D3                GENMASK(6, 4)
#define  DP_ANA_TX_PRESEL_D2                GENMASK(2, 0)

#define DP_ANA_TX_CTRL6                      0x01bc
#define  DP_ANA_BG_VREF5                    GENMASK(15, 14)
#define  DP_ANA_BG_VREF4                    GENMASK(13, 12)
#define  DP_ANA_BG_VREF3                    GENMASK(11, 10)
#define  DP_ANA_BG_VREF2                    GENMASK(9, 8)
#define  DP_ANA_BG_VREF                     GENMASK(7, 6)
#define  DP_ANA_BG_VREF1                    GENMASK(5, 4)
#define  DP_ANA_BG_EN_CHOP                  BIT(3)
#define  DP_ANA_BG_EN                       BIT(2)
#define  DP_ANA_BG_ISEL                     BIT(1)
#define  DP_ANA_BG_VSEL                     BIT(0)

#define DP_ANA_TX_CTRL7                      0x01c0
#define  DP_ANA_BG_EN_RCAL                  BIT(27)
#define  DP_ANA_BG_RCAL_SEL                 GENMASK(26, 25)
#define  DP_AD_BG_RCAL_OUT                  BIT(24)
#define  DP_ANA_RTCAL_FREQDIV_LBIT          GENMASK(23, 16)
#define  DP_ANA_RTCAL_BYPASS                BIT(15)
#define  DP_ANA_RTCAL_FREQDIV_HBIT          GENMASK(14, 8)
#define  DP_ANA_BG_RCAL_VAL                 GENMASK(5, 0)

#define DP_ANA_TX_CTRL8                      0x01c4
#define  DP_ANA_TX_RTM_D3                   GENMASK(29, 24)
#define  DP_ANA_TX_RTM_D2                   GENMASK(21, 16)
#define  DP_ANA_TX_RTM_D1                   GENMASK(13, 8)
#define  DP_ANA_TX_RTM_D0                   GENMASK(5, 0)

#define DP_ANA_TX_AUX_CTRL                   0x01d0
#define  DP_ANA_TX_AUX_RX_VSEL              GENMASK(13, 12)

#define DP_ANA_LANE_EMPH                     0x01dc
#define  DP_ANA_TX_POSTSEL_PRE_D3           GENMASK(23, 20)
#define  DP_ANA_TX_POSTSEL_PRE_D2           GENMASK(19, 16)
#define  DP_ANA_TX_POSTSEL_PRE_D1           GENMASK(15, 12)
#define  DP_ANA_TX_POSTSEL_PRE_D0           GENMASK(11, 8)
#define  DP_ANA_TX_MODE_DE_D3               BIT(7)
#define  DP_ANA_TX_MODE_DE_D2               BIT(6)
#define  DP_ANA_TX_MODE_DE_D1               BIT(5)
#define  DP_ANA_TX_MODE_DE_D0               BIT(4)
#define  DP_ANA_TX_MODE_PRE_D3              BIT(3)
#define  DP_ANA_TX_MODE_PRE_D2              BIT(2)
#define  DP_ANA_TX_MODE_PRE_D1              BIT(1)
#define  DP_ANA_TX_MODE_PRE_D0              BIT(0)

#define DP_VIDEO_VSAMPLE_REG                 0x0200
#define  DP_VIDEO_STREAM_ENABLE             BIT(28)
#define  DP_VIDEO_MAPPING                   GENMASK(26, 22)
#define  DP_STREAM_ENC_EN                   BIT(18)

#define DP_VINPUT_POLARITY                   0x020c
#define  DP_HSYNC_IN_POLARITY               BIT(29)
#define  DP_VSYNC_IN_POLARITY               BIT(28)

#define DP_VIDEO_CONFIG2                     0x0210
#define  DP_VACTIVE                         GENMASK(31, 16)
#define  DP_VBLANK                          GENMASK(15, 0)

#define DP_VIDEO_CONFIG1                     0x0214
#define  DP_HBLANK                          GENMASK(29, 16)
#define  DP_HACTIVE                         GENMASK(15, 0)

#define DP_VIDEO_CONFIG5                     0x0218
#define  DP_AVERAGE_BYTES_PER_TU            GENMASK(31, 25)
#define  DP_AVERAGE_BYTES_PER_TU_FRAC       GENMASK(24, 21)
#define  DP_INIT_THRESHOLD                  GENMASK(18, 10)

#define DP_VIDEO_CONFIG3                     0x021c
#define  DP_H_FRONT_PORCH                   GENMASK(31, 16)
#define  DP_H_SYNC_WIDTH                    GENMASK(15, 0)

#define DP_VIDEO_CONFIG4                     0x0220
#define  DP_V_FRONT_PORCH                   GENMASK(31, 16)
#define  DP_V_SYNC_WIDTH                    GENMASK(15, 0)

#define DP_VIDEO_MSA1                        0x0224
#define  DP_HSTART                          GENMASK(31, 16)
#define  DP_VSTART                          GENMASK(15, 0)

#define DP_VIDEO_MSA2                        0x0228
#define  DP_MISC0                           GENMASK(7, 0)

#define DP_VIDEO_MSA3                        0x022c
#define  DP_NVID                            GENMASK(31, 8)
#define  DP_MISC1                           GENMASK(7, 0)

#define DP_VIDEO_HBLANK_INTERVAL             0x0230
#define  DP_HBLANK_INTERVAL                 GENMASK(31, 16)

#define DP_VIDEO_COLORBAR_CFG                0x0238
#define  DP_VID_BIST_EN                     BIT(0)

#define DP_AUDIO_CONFIG1                     0x0300
#define  DP_AUDIO_INF_SELECT                BIT(31)
#define  DP_AUDIO_MUTE                      BIT(30)
#define  DP_HBR_MODE_ENABLE                 BIT(29)
#define  DP_AUDIO_DATA_IN_EN                GENMASK(28, 25)
#define  DP_I2S_AUDIO_MODE                  GENMASK(24, 23)
#define  DP_AUD_ADJUST_SEL                  BIT(22)
#define  DP_AUDIO_DATA_WIDTH                GENMASK(21, 17)
#define  DP_AUDIO_NUM_CHANNELS              GENMASK(16, 14)
#define  DP_AUDIO_PACKET_ID                 GENMASK(13, 6)
#define  DP_AUDIO_TIMESTAMP_VERSION_NUM     GENMASK(5, 0)

#define DP_AUX_STS_REG                       0x0400
#define  DP_AUX_STATUS                      GENMASK(31, 24)
#define  DP_AUX_REPLY_ERR_CODE              GENMASK(6, 4)

#define DP_AUX_CMD_REG                       0x0404
#define  DP_AUX_LENGTH                      GENMASK(27, 24)
#define  DP_AUX_ADDR                        GENMASK(23, 4)
#define  DP_AUX_CMD_TYPE                    GENMASK(3, 0)

#define DP_AUX_START_REG                     0x0408
#define  DP_AUX_START                       BIT(31)

#define DP_AUX_DATA4_REG                     0x040c
#define  DP_AUX_DATA4                       GENMASK(31, 0)

#define DP_AUX_DATA3_REG                     0x0410
#define  DP_AUX_DATA3                       GENMASK(31, 0)

#define DP_AUX_DATA2_REG                     0x0414
#define  DP_AUX_DATA2                       GENMASK(31, 0)

#define DP_AUX_DATA1_REG                     0x0418
#define  DP_AUX_DATA1                       GENMASK(31, 0)

#define DP_SDP_HORIZONTAL_CTRL               0x0420
#define  DP_AUD_STREAM_HORIZONTAL_EN        BIT(13)
#define  DP_AUD_TIMESTAMP_HORIZONTAL_EN     BIT(12)

#define DP_SDP_VERTICAL_CTRL                 0x0424
#define  DP_AUD_STREAM_VERTICAL_EN          BIT(13)
#define  DP_AUD_TIMESTAMP_VERTICAL_EN       BIT(12)

#endif /* __SPACEMIT_INNO_DP_H__ */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef _SATURN_REG_MAP_hee_H_
#define _SATURN_REG_MAP_hee_H_

#include "dpu_int.h"

/* Base address, then bit definitions */

#define DPU_TOP_BASE_ADDR 0x0
#define DPU_CTRL_BASE_ADDR 0x3c0 /* dpu_ctl_top start address */
#define CMDLIST_BASE_ADDR 0x500
#define DPU_INT_BASE_ADDR 0x700

#define DPU_ONLINE_IRQ_MSK	(3 * 4)
#define DPU_ONLINE_IRQ_STS	(12 * 4)

#define DMA_TOP_BASE_ADDR (RDMA0_BASE_ADDR + 0x280)

#define RDMA0_BASE_ADDR (0x00001000)
#define RDMA1_BASE_ADDR (RDMA0_BASE_ADDR + 0x1000)
#define RDMA2_BASE_ADDR (RDMA0_BASE_ADDR + 0xa000)
#define RDMA3_BASE_ADDR (RDMA0_BASE_ADDR + 0xb000)

#define MMU_BASE_ADDR (0x1100)
#define MMU_TOP_BASE_ADDR (MMU_BASE_ADDR + 0x100)

#define TBU0_ADDR 0x1100
#define TBU1_ADDR 0x1180
#define TBU2_ADDR 0x2100
#define TBU3_ADDR 0x2180
#define TBU4_ADDR 0xb100
#define TBU5_ADDR 0xb180
#define TBU6_ADDR 0xc100
#define TBU7_ADDR 0xc180

#define CMP0_BASE_ADDR 0x00018000
#define CMP1_BASE_ADDR (CMP0_BASE_ADDR + 0x18000)
#define CMP2_BASE_ADDR (CMP0_BASE_ADDR + 0x30000)

#define PREPIPE_SCAL0_BASE_ADDR 0x14000
#define PREPIPE_SCAL1_BASE_ADDR 0x14400
#define PREPIPE_SCAL2_BASE_ADDR 0x14800
#define PREPIPE_SCAL3_BASE_ADDR 0x14c00

#define POST_PIPE_ADDR (CMP1_BASE_ADDR + 0x300)

#define EE_ADDR (CMP1_BASE_ADDR + 0xd00)

#define DPU_SCENE_CTL_BASE_ADDR (0x300)
#define DPU_SCENE_CTL_SIZE 0x40
#define DPU_SCENE_CTL_ADDR(i) (DPU_SCENE_CTL_BASE_ADDR + (i) * DPU_SCENE_CTL_SIZE)

#define TMG0_BASE_ADDR (0x00051000 + 0x200)

#define CMDLIST_CH_Y                 0x188
#define CMDLIST_CH_START_CMPS_Y      0x90
#define CMDLIST_CFG_READY            0x34

#endif

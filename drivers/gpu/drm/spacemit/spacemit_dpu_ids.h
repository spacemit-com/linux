/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 */

#ifndef _SPACEMIT_DPU_IDS_H_
#define _SPACEMIT_DPU_IDS_H_

/* Saturn online compositor instance ids (hardware IRQ-table indices). */
#define COMPOSER0	0
#define COMPOSER1	1
#define COMPOSER2	2
#define COMPOSER3	3

/* DPU output-format field encoding (dpuctrl out-format register). */
#define OUTFMT_RGB121212	0
#define OUTFMT_RGB101010	1
#define OUTFMT_RGB888		2
#define OUTFMT_RGB666		12
#define OUTFMT_RGB565		13

#endif /* _SPACEMIT_DPU_IDS_H_ */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 * Saturn DPU register accessors.
 */

#ifndef _SPACEMIT_DPU_REG_H_
#define _SPACEMIT_DPU_REG_H_

#include <linux/bitfield.h>
#include <linux/io.h>

#include "dpu/dpu_trace.h"
#include "dpu/saturn_regs/saturn_fields.h"

#define dpu_field_word(module_name, field)	(module_name##__##field##_WORD)
#define dpu_field_mask(module_name, field)	(module_name##__##field##_MASK)

/* MMIO field read: returns the field value shifted down to bit 0. */
#define dpu_read_reg(hwdev, module_name, module_base, field)		\
	FIELD_GET(dpu_field_mask(module_name, field),			\
		  readl((hwdev)->base + (module_base) +			\
			4 * dpu_field_word(module_name, field)))

/* MMIO raw 32-bit register read/write by word index within the block. */
#define dpu_read_word(hwdev, module_base, word)				\
	readl((hwdev)->base + (module_base) + 4 * (word))

#define dpu_write_reg_word(hwdev, module_base, word, data)		\
do {									\
	trace_spacemit_dpu_reg_write((u32)(data), (module_base), NULL);	\
	writel((u32)(data),						\
	       (hwdev)->base + (module_base) + 4 * (word));		\
} while (0)

/*
 * Not every DPU config word reads back what was written, so a byte- or
 * halfword-aligned field is stored at its own width rather than read-modified.
 */
#define __dpu_mmio_field_write(__addr, __mask, data)			\
do {									\
	if ((__mask) == 0xffU)						\
		writeb((u8)((data) & 0xffU), __addr);			\
	else if ((__mask) == 0xff00U)					\
		writeb((u8)((data) & 0xffU), (__addr) + 1);		\
	else if ((__mask) == 0xff0000U)					\
		writeb((u8)((data) & 0xffU), (__addr) + 2);		\
	else if ((__mask) == 0xff000000U)				\
		writeb((u8)((data) & 0xffU), (__addr) + 3);		\
	else if ((__mask) == 0xffffU)					\
		writew((u16)((data) & 0xffffU), __addr);		\
	else if ((__mask) == 0xffff0000U)				\
		writew((u16)((data) & 0xffffU), (__addr) + 2);		\
	else if ((__mask) == 0xffffffffU)				\
		writel((u32)(data), __addr);				\
	else								\
		writel((readl(__addr) & ~(__mask)) |			\
		       FIELD_PREP(__mask, (u32)(data)), __addr);	\
} while (0)

#define dpu_write_reg(hwdev, module_name, module_base, field, data)	\
do {									\
	void __iomem *__waddr = (hwdev)->base + (module_base) +	\
				4 * dpu_field_word(module_name, field);	\
									\
	trace_spacemit_dpu_reg_write((u32)(data), (module_base), #field); \
	__dpu_mmio_field_write(__waddr,					\
			       dpu_field_mask(module_name, field), data); \
} while (0)

/* Field write into a register array element; the index is a runtime value. */
#define dpu_write_reg_arr(hwdev, module_name, module_base, arr, i, field, data) \
do {									\
	void __iomem *__waddr = (hwdev)->base + (module_base) +	\
		4 * module_name##__##arr##__##field##_WORD(i);		\
									\
	trace_spacemit_dpu_reg_write((u32)(data), (module_base), #field); \
	__dpu_mmio_field_write(__waddr,					\
			       module_name##__##arr##__##field##_MASK,	\
			       data);					\
} while (0)

/*
 * Unified CPU/cmdlist field write; a NULL cl_regs selects the direct MMIO path.
 */
#define _dpu_write_reg(hwdev, module_name, module_base, field, data, cl_regs, ...) \
do {									\
	if ((cl_regs) != NULL)						\
		cmdlist_write_reg(hwdev, module_name, module_base,	\
				  field, data, cl_regs);		\
	else								\
		dpu_write_reg(hwdev, module_name, module_base, field,	\
			      data);					\
} while (0)

#define dpu_write(...) _dpu_write_reg(__VA_ARGS__, NULL)

/* Unified CPU/cmdlist raw-word write (the value32[] call sites). */
#define dpu_write_word(hwdev, module_base, word, data, cl_regs)	\
do {									\
	if ((cl_regs) != NULL)						\
		cmdlist_write_word(module_base, word, data, cl_regs);	\
	else								\
		dpu_write_reg_word(hwdev, module_base, word, data);	\
} while (0)

#endif

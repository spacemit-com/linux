/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 */

#ifndef _SATURN_FW_CMDLIST_H_
#define _SATURN_FW_CMDLIST_H_

#include <linux/stddef.h>
#include <linux/types.h>
#include <drm/drm_crtc.h>
#include <drm/drm_plane.h>

struct spacemit_crtc;
struct spacemit_plane_state;

#define PER_CMDLIST_SIZE             4096
#define CMDLIST_ROW_REGS	(3)
#define CMDLIST_CMP_INVALID	(0xff)
/* poison for the per-cmdlist compose sentinels, never written to hw */
#define CMDLIST_SENTINEL_POISON	0xDEADBEEF

/**
 * enum cmdlist_type - what surface owns a cmdlist instance
 * @CMDLIST_PLANE: Per-plane RDMA register batches.
 * @CMDLIST_CRTC: Compose-side batches submitted on every flush.
 */
enum cmdlist_type {
	CMDLIST_PLANE = 1,
	CMDLIST_CRTC,
};

/**
 * struct cmdlist - A DMA-backed batch of register writes replayed at CFG_RDY
 * @nod_len: Number of register-write rows currently filled.
 * @size: Capacity of the DMA buffer in bytes.
 * @mode_mask: cmdlist_mode_type sources; gates which rows are pulled.
 * @va: Kernel virtual address of the DMA buffer.
 * @pa: DMA address programmed into the cmdlist base register.
 * @next: Cmdlists in submission order; the CRTC-level one comes last.
 * @type: Originating surface; see &enum cmdlist_type.
 * @rch_start_cmps_y: Compositor-Y start sentinel.
 * @cmdlist_ch_y_other: Companion sentinel to @rch_start_cmps_y.
 */
struct cmdlist {
	u16 nod_len;
	u32 size;
	u32 mode_mask;
	void *va;
	dma_addr_t pa;
	struct cmdlist *next;
	u8 type;
	u32 rch_start_cmps_y;
	u32 cmdlist_ch_y_other;
};

enum cmdlist_mode_type {
	CMDLIST_MOD_RDMA = BIT(0),
	CMDLIST_MOD_COMP = BIT(3),
	CMDLIST_MOD_DMMU = BIT(6),
};

#define CMDLIST_REG_STROBE(index)	(0xf << ((index) * 4))

#define CMDLIST_ADDRL_ALIGN_BITS               (4) /* From cmdlist_reg_0[] in CMDLIST_REG */
#define CMDLIST_ADDRL_ALIGN_MASK \
	((u32)(~(BIT(CMDLIST_ADDRL_ALIGN_BITS) - 1)))
/* NULL on failure is safe: every consumer falls back to direct MMIO. */
#define alloc_cmdlist_regs(module_name) \
({ \
	struct cmdlist_regs *cl = kzalloc(sizeof(*cl), GFP_KERNEL); \
	if (cl) { \
		cl->size = module_name##__NWORDS; \
		cl->module = kzalloc(4 * module_name##__NWORDS, GFP_KERNEL); \
		cl->flags = kzalloc(module_name##__NWORDS, GFP_KERNEL); \
		if (!cl->module || !cl->flags) { \
			kfree(cl->module); \
			kfree(cl->flags); \
			kfree(cl); \
			cl = NULL; \
		} \
	} \
	cl; \
})

#define free_cmdlist_regs(cl_p) \
do { \
	struct cmdlist_regs *cl = (cl_p); \
	if (cl) { \
		kfree(cl->module); \
		kfree(cl->flags); \
		kfree(cl); \
	} \
} while (0)

/*
 * First touch seeds the word from a live MMIO read: only register blocks that
 * read back what was written may be routed through cmdlist.
 */
#define cmdlist_write_reg(hwdev, module_name, module_base, field, data, cl_regs) \
do { \
	struct cmdlist_regs *cl_p = (cl_regs); \
	u32 *__words = cl_p->module; \
	const unsigned int __w = dpu_field_word(module_name, field); \
	if (cl_p->flags[__w] == 0) { \
		__words[__w] = readl((hwdev)->base + (module_base) + 4 * __w); \
		cl_p->base = (module_base); \
		cl_p->flags[__w] = 1; \
	} \
	__words[__w] &= ~dpu_field_mask(module_name, field); \
	__words[__w] |= FIELD_PREP(dpu_field_mask(module_name, field), \
				   (u32)(data)); \
} while (0)

/* Raw whole-word variant (the value32[] call sites); no seeding needed. */
#define cmdlist_write_word(module_base, word, data, cl_regs) \
do { \
	struct cmdlist_regs *cl_p = (cl_regs); \
	u32 *__words = cl_p->module; \
	cl_p->base = (module_base); \
	cl_p->flags[(word)] = 1; \
	__words[(word)] = (u32)(data); \
} while (0)

struct cmdlist_regs {
	u32 base;
	u16 size;
	u32 *module;
	u8 *flags;
};

void cmdlist_regs_packing(struct cmdlist *cl, enum cmdlist_mode_type mod,
			  struct cmdlist_regs *cl_regs);
void cmdlist_sort_by_group(struct drm_crtc *crtc);
void cmdlist_atomic_commit(struct drm_crtc *crtc,
			   struct drm_crtc_state *old_state);
struct spacemit_plane_state *cl_to_spacemit_pstate(const struct cmdlist *cl);
void crtc_cmdlist_sort_by_group(struct spacemit_crtc *a_crtc);
void crtc_cmdlist_atomic_commit(struct spacemit_crtc *a_crtc);
struct spacemit_crtc_state *cl_to_spacemit_cstate(const struct cmdlist *cl);
int spacemit_cmdlist_alloc(struct device *dev, struct cmdlist *cl);
struct cmdlist *plane_to_cl(struct drm_plane *plane);
struct cmdlist *crtc_to_cl(struct drm_crtc *crtc);

#endif /* _SATURN_FW_CMDLIST_H_ */

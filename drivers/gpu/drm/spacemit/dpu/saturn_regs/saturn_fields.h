/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025-2026 SpacemiT Co., Ltd.
 *
 * Saturn DPU register field map: word offset and mask per field, consumed by
 * the dpu_write()/dpu_read() accessors.
 */

#ifndef SATURN_FIELDS_H
#define SATURN_FIELDS_H

#include <linux/bits.h>

/* CMDLIST_REG: 128 x 32-bit registers */
#define CMDLIST_REG__NWORDS	128
#define CMDLIST_REG__cmdlist_reg_0__cmdlist_ch_start_addrl_WORD(i)	(0 + (i))
#define CMDLIST_REG__cmdlist_reg_0__cmdlist_ch_start_addrl_MASK	GENMASK(31, 4)
#define CMDLIST_REG__cmdlist_reg_16__cmdlist_ch_start_addrh_WORD(i)	(16 + (i))
#define CMDLIST_REG__cmdlist_reg_16__cmdlist_ch_start_addrh_MASK	GENMASK(5, 0)
#define CMDLIST_REG__cmdlist_reg_48__cmdlist_ch_y_first_WORD(i)	(48 + (i))
#define CMDLIST_REG__cmdlist_reg_48__cmdlist_ch_y_first_MASK	GENMASK(24, 12)

/* CMPS_X_REG: 146 x 32-bit registers */
#define CMPS_X_REG__NWORDS	146
#define CMPS_X_REG__module_enable_WORD	0
#define CMPS_X_REG__module_enable_MASK	GENMASK(0, 0)
#define CMPS_X_REG__dst_w_WORD	1
#define CMPS_X_REG__dst_w_MASK	GENMASK(15, 0)
#define CMPS_X_REG__dst_h_WORD	1
#define CMPS_X_REG__dst_h_MASK	GENMASK(31, 16)
#define CMPS_X_REG__bg_color_r_WORD	2
#define CMPS_X_REG__bg_color_r_MASK	GENMASK(9, 0)
#define CMPS_X_REG__bg_color_g_WORD	3
#define CMPS_X_REG__bg_color_g_MASK	GENMASK(9, 0)
#define CMPS_X_REG__bg_color_b_WORD	4
#define CMPS_X_REG__bg_color_b_MASK	GENMASK(9, 0)
#define CMPS_X_REG__bg_color_a_WORD	5
#define CMPS_X_REG__bg_color_a_MASK	GENMASK(7, 0)
#define CMPS_X_REG__layer00_area_left_WORD	8
#define CMPS_X_REG__layer00_area_left_MASK	GENMASK(15, 0)
#define CMPS_X_REG__layer00_area_right_WORD	8
#define CMPS_X_REG__layer00_area_right_MASK	GENMASK(31, 16)
#define CMPS_X_REG__layer00_area_top_WORD	9
#define CMPS_X_REG__layer00_area_top_MASK	GENMASK(15, 0)
#define CMPS_X_REG__layer00_area_bottom_WORD	9
#define CMPS_X_REG__layer00_area_bottom_MASK	GENMASK(31, 16)
#define CMPS_X_REG__layer00_solid_color_r_WORD	10
#define CMPS_X_REG__layer00_solid_color_r_MASK	GENMASK(9, 0)
#define CMPS_X_REG__layer00_solid_color_g_WORD	11
#define CMPS_X_REG__layer00_solid_color_g_MASK	GENMASK(9, 0)
#define CMPS_X_REG__layer00_solid_color_b_WORD	12
#define CMPS_X_REG__layer00_solid_color_b_MASK	GENMASK(9, 0)
#define CMPS_X_REG__layer00_solid_color_a_WORD	13
#define CMPS_X_REG__layer00_solid_color_a_MASK	GENMASK(7, 0)
#define CMPS_X_REG__layer00_blend_mode_WORD	13
#define CMPS_X_REG__layer00_blend_mode_MASK	GENMASK(15, 14)
#define CMPS_X_REG__layer00_colorkey_en_WORD	13
#define CMPS_X_REG__layer00_colorkey_en_MASK	GENMASK(18, 18)
#define CMPS_X_REG__layer00_en_WORD	14
#define CMPS_X_REG__layer00_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer00_blend_sel_WORD	14
#define CMPS_X_REG__layer00_blend_sel_MASK	GENMASK(2, 1)
#define CMPS_X_REG__layer00_solid_en_WORD	14
#define CMPS_X_REG__layer00_solid_en_MASK	GENMASK(3, 3)
#define CMPS_X_REG__layer00_dma_id_WORD	14
#define CMPS_X_REG__layer00_dma_id_MASK	GENMASK(7, 4)
#define CMPS_X_REG__layer00_layer_alpha_WORD	14
#define CMPS_X_REG__layer00_layer_alpha_MASK	GENMASK(15, 8)
#define CMPS_X_REG__layer00_alpha_ratio_WORD	14
#define CMPS_X_REG__layer00_alpha_ratio_MASK	GENMASK(23, 16)
#define CMPS_X_REG__layer01_en_WORD	21
#define CMPS_X_REG__layer01_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer02_en_WORD	28
#define CMPS_X_REG__layer02_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer03_en_WORD	35
#define CMPS_X_REG__layer03_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer04_en_WORD	42
#define CMPS_X_REG__layer04_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer05_en_WORD	49
#define CMPS_X_REG__layer05_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer06_en_WORD	56
#define CMPS_X_REG__layer06_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer07_en_WORD	63
#define CMPS_X_REG__layer07_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer08_en_WORD	70
#define CMPS_X_REG__layer08_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer09_en_WORD	77
#define CMPS_X_REG__layer09_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer10_en_WORD	84
#define CMPS_X_REG__layer10_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer11_en_WORD	91
#define CMPS_X_REG__layer11_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer12_en_WORD	98
#define CMPS_X_REG__layer12_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer13_en_WORD	105
#define CMPS_X_REG__layer13_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer14_en_WORD	112
#define CMPS_X_REG__layer14_en_MASK	GENMASK(0, 0)
#define CMPS_X_REG__layer15_en_WORD	119
#define CMPS_X_REG__layer15_en_MASK	GENMASK(0, 0)

/* DMA_TOP_REG: 25 x 32-bit registers */
#define DMA_TOP_REG__NWORDS	25
#define DMA_TOP_REG__image_rr_ratio_WORD	1
#define DMA_TOP_REG__image_rr_ratio_MASK	GENMASK(7, 0)
#define DMA_TOP_REG__pixel_num_th_WORD	1
#define DMA_TOP_REG__pixel_num_th_MASK	GENMASK(14, 9)
#define DMA_TOP_REG__rdma_timeout_limit_WORD	2
#define DMA_TOP_REG__rdma_timeout_limit_MASK	GENMASK(15, 0)
#define DMA_TOP_REG__wdma_timeout_limit_WORD	2
#define DMA_TOP_REG__wdma_timeout_limit_MASK	GENMASK(31, 16)

/* DPU_CTL_REG: 15 x 32-bit registers */
#define DPU_CTL_REG__NWORDS	15
#define DPU_CTL_REG__nml_rch_en_WORD	0
#define DPU_CTL_REG__nml_rch_en_MASK	GENMASK(9, 0)
#define DPU_CTL_REG__nml_scl_en_WORD	0
#define DPU_CTL_REG__nml_scl_en_MASK	GENMASK(13, 10)
#define DPU_CTL_REG__nml_outctl_en_WORD	0
#define DPU_CTL_REG__nml_outctl_en_MASK	GENMASK(18, 18)
#define DPU_CTL_REG__nml_frm_timing_en_WORD	0
#define DPU_CTL_REG__nml_frm_timing_en_MASK	GENMASK(25, 23)
#define DPU_CTL_REG__both_cfg_rdy_WORD	2
#define DPU_CTL_REG__both_cfg_rdy_MASK	GENMASK(0, 0)
#define DPU_CTL_REG__video_mod_WORD	3
#define DPU_CTL_REG__video_mod_MASK	GENMASK(0, 0)
#define DPU_CTL_REG__dbg_mod_WORD	3
#define DPU_CTL_REG__dbg_mod_MASK	GENMASK(1, 1)
#define DPU_CTL_REG__timing_inter0_WORD	3
#define DPU_CTL_REG__timing_inter0_MASK	GENMASK(7, 2)
#define DPU_CTL_REG__timing_inter1_WORD	3
#define DPU_CTL_REG__timing_inter1_MASK	GENMASK(13, 8)
#define DPU_CTL_REG__sw_start_WORD	4
#define DPU_CTL_REG__sw_start_MASK	GENMASK(0, 0)
#define DPU_CTL_REG__nml_rch_vrt_reuse_WORD	7
#define DPU_CTL_REG__nml_rch_vrt_reuse_MASK	GENMASK(9, 0)

/* DPU_CTL_TOP_REG: 53 x 32-bit registers */
#define DPU_CTL_TOP_REG__NWORDS	53
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_0__cmdlist_rch_en_WORD(i)	(0 + (i))
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_0__cmdlist_rch_en_MASK	GENMASK(0, 0)
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_10__cmdlist_cmps_other_en_WORD(i)	(10 + (i))
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_10__cmdlist_cmps_other_en_MASK	GENMASK(0, 0)
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_10__cmdlist_cmps_top_en_WORD(i)	(10 + (i))
#define DPU_CTL_TOP_REG__dpu_ctl_top_reg_10__cmdlist_cmps_top_en_MASK	GENMASK(1, 1)

/* DPU_INTP_REG: 50 x 32-bit registers */
#define DPU_INTP_REG__NWORDS	50

/* EE_REG: 51 x 32-bit registers */
#define EE_REG__NWORDS	51
#define EE_REG__m_benable_WORD	0
#define EE_REG__m_benable_MASK	GENMASK(0, 0)

/* MMU_TBU_REG: 21 x 32-bit registers */
#define MMU_TBU_REG__NWORDS	21
#define MMU_TBU_REG__tbu_base_addr0_low_WORD	1
#define MMU_TBU_REG__tbu_base_addr0_low_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_base_addr0_high_WORD	2
#define MMU_TBU_REG__tbu_base_addr0_high_MASK	GENMASK(11, 0)
#define MMU_TBU_REG__tbu_base_addr1_low_WORD	3
#define MMU_TBU_REG__tbu_base_addr1_low_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_base_addr1_high_WORD	4
#define MMU_TBU_REG__tbu_base_addr1_high_MASK	GENMASK(11, 0)
#define MMU_TBU_REG__tbu_base_addr2_low_WORD	5
#define MMU_TBU_REG__tbu_base_addr2_low_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_base_addr2_high_WORD	6
#define MMU_TBU_REG__tbu_base_addr2_high_MASK	GENMASK(11, 0)
#define MMU_TBU_REG__tbu_va0_WORD	7
#define MMU_TBU_REG__tbu_va0_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_va1_WORD	8
#define MMU_TBU_REG__tbu_va1_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_va2_WORD	9
#define MMU_TBU_REG__tbu_va2_MASK	GENMASK(31, 0)
#define MMU_TBU_REG__tbu_size0_WORD	10
#define MMU_TBU_REG__tbu_size0_MASK	GENMASK(15, 0)
#define MMU_TBU_REG__tbu_size1_WORD	11
#define MMU_TBU_REG__tbu_size1_MASK	GENMASK(15, 0)
#define MMU_TBU_REG__tbu_size2_WORD	12
#define MMU_TBU_REG__tbu_size2_MASK	GENMASK(15, 0)
#define MMU_TBU_REG__vsync_update_en_WORD	13
#define MMU_TBU_REG__vsync_update_en_MASK	GENMASK(1, 1)

/* MMU_TOP_REG: 23 x 32-bit registers */
#define MMU_TOP_REG__NWORDS	23
#define MMU_TOP_REG__rdma_timelimit_WORD	0
#define MMU_TOP_REG__rdma_timelimit_MASK	GENMASK(31, 16)
#define MMU_TOP_REG__sram0_tlb_axi_port_sel_WORD	1
#define MMU_TOP_REG__sram0_tlb_axi_port_sel_MASK	GENMASK(1, 0)
#define MMU_TOP_REG__sram1_tlb_axi_port_sel_WORD	1
#define MMU_TOP_REG__sram1_tlb_axi_port_sel_MASK	GENMASK(3, 2)
#define MMU_TOP_REG__dmac0_rd_outs_num_WORD	2
#define MMU_TOP_REG__dmac0_rd_outs_num_MASK	GENMASK(7, 0)
#define MMU_TOP_REG__dmac1_rd_outs_num_WORD	3
#define MMU_TOP_REG__dmac1_rd_outs_num_MASK	GENMASK(7, 0)

/* POSTPIPE_REG: 50 x 32-bit registers */
#define POSTPIPE_REG__NWORDS	50
#define POSTPIPE_REG__m_inwidth_WORD	13
#define POSTPIPE_REG__m_inwidth_MASK	GENMASK(15, 0)
#define POSTPIPE_REG__m_inheight_WORD	13
#define POSTPIPE_REG__m_inheight_MASK	GENMASK(31, 16)

/* RDMA_PATH_X_REG: 60 x 32-bit registers */
#define RDMA_PATH_X_REG__NWORDS	60
#define RDMA_PATH_X_REG__layer_mode_WORD	0
#define RDMA_PATH_X_REG__layer_mode_MASK	GENMASK(1, 0)
#define RDMA_PATH_X_REG__layer_cmpsr_id_WORD	0
#define RDMA_PATH_X_REG__layer_cmpsr_id_MASK	GENMASK(15, 13)
#define RDMA_PATH_X_REG__is_two_layers_WORD	1
#define RDMA_PATH_X_REG__is_two_layers_MASK	GENMASK(0, 0)
#define RDMA_PATH_X_REG__is_offline_WORD	1
#define RDMA_PATH_X_REG__is_offline_MASK	GENMASK(1, 1)
#define RDMA_PATH_X_REG__base_addr0_low_ly0_WORD	9
#define RDMA_PATH_X_REG__base_addr0_low_ly0_MASK	GENMASK(31, 0)
#define RDMA_PATH_X_REG__base_addr0_high_ly0_WORD	10
#define RDMA_PATH_X_REG__base_addr0_high_ly0_MASK	GENMASK(1, 0)
#define RDMA_PATH_X_REG__base_addr1_low_ly0_WORD	11
#define RDMA_PATH_X_REG__base_addr1_low_ly0_MASK	GENMASK(31, 0)
#define RDMA_PATH_X_REG__base_addr1_high_ly0_WORD	12
#define RDMA_PATH_X_REG__base_addr1_high_ly0_MASK	GENMASK(1, 0)
#define RDMA_PATH_X_REG__base_addr2_low_ly0_WORD	13
#define RDMA_PATH_X_REG__base_addr2_low_ly0_MASK	GENMASK(31, 0)
#define RDMA_PATH_X_REG__base_addr2_high_ly0_WORD	14
#define RDMA_PATH_X_REG__base_addr2_high_ly0_MASK	GENMASK(1, 0)
#define RDMA_PATH_X_REG__rdma_stride0_layer0_WORD	15
#define RDMA_PATH_X_REG__rdma_stride0_layer0_MASK	GENMASK(15, 0)
#define RDMA_PATH_X_REG__rdma_stride1_layer0_WORD	15
#define RDMA_PATH_X_REG__rdma_stride1_layer0_MASK	GENMASK(31, 16)
#define RDMA_PATH_X_REG__img_width_ly0_WORD	16
#define RDMA_PATH_X_REG__img_width_ly0_MASK	GENMASK(15, 0)
#define RDMA_PATH_X_REG__img_height_ly0_WORD	16
#define RDMA_PATH_X_REG__img_height_ly0_MASK	GENMASK(31, 16)
#define RDMA_PATH_X_REG__bbox_start_x_ly0_WORD	17
#define RDMA_PATH_X_REG__bbox_start_x_ly0_MASK	GENMASK(15, 0)
#define RDMA_PATH_X_REG__bbox_start_y_ly0_WORD	17
#define RDMA_PATH_X_REG__bbox_start_y_ly0_MASK	GENMASK(31, 16)
#define RDMA_PATH_X_REG__bbox_end_x_ly0_WORD	18
#define RDMA_PATH_X_REG__bbox_end_x_ly0_MASK	GENMASK(15, 0)
#define RDMA_PATH_X_REG__bbox_end_y_ly0_WORD	18
#define RDMA_PATH_X_REG__bbox_end_y_ly0_MASK	GENMASK(31, 16)
#define RDMA_PATH_X_REG__pixel_format_WORD	29
#define RDMA_PATH_X_REG__pixel_format_MASK	GENMASK(5, 0)
#define RDMA_PATH_X_REG__uv_swap_WORD	29
#define RDMA_PATH_X_REG__uv_swap_MASK	GENMASK(6, 6)
#define RDMA_PATH_X_REG__rot_mode_ly0_WORD	29
#define RDMA_PATH_X_REG__rot_mode_ly0_MASK	GENMASK(10, 8)

/* TMG_REG: 30 x 32-bit registers */
#define TMG_REG__NWORDS	30
#define TMG_REG__split_en_WORD	0
#define TMG_REG__split_en_MASK	GENMASK(0, 0)
#define TMG_REG__cmd_screen_WORD	0
#define TMG_REG__cmd_screen_MASK	GENMASK(1, 1)
#define TMG_REG__fm_timing_en_WORD	0
#define TMG_REG__fm_timing_en_MASK	GENMASK(2, 2)
#define TMG_REG__cmd_wait_en_WORD	0
#define TMG_REG__cmd_wait_en_MASK	GENMASK(3, 3)
#define TMG_REG__cmd_wait_te_WORD	0
#define TMG_REG__cmd_wait_te_MASK	GENMASK(4, 4)
#define TMG_REG__disp_ready_man_en_WORD	0
#define TMG_REG__disp_ready_man_en_MASK	GENMASK(5, 5)
#define TMG_REG__hsp_WORD	0
#define TMG_REG__hsp_MASK	GENMASK(7, 7)
#define TMG_REG__vsp_WORD	0
#define TMG_REG__vsp_MASK	GENMASK(8, 8)
#define TMG_REG__user_WORD	2
#define TMG_REG__user_MASK	GENMASK(15, 12)
#define TMG_REG__hfp_WORD	3
#define TMG_REG__hfp_MASK	GENMASK(27, 16)
#define TMG_REG__hsync_width_WORD	4
#define TMG_REG__hsync_width_MASK	GENMASK(9, 0)
#define TMG_REG__hbp_WORD	4
#define TMG_REG__hbp_MASK	GENMASK(27, 16)
#define TMG_REG__vfp_WORD	5
#define TMG_REG__vfp_MASK	GENMASK(15, 0)
#define TMG_REG__vsync_width_WORD	5
#define TMG_REG__vsync_width_MASK	GENMASK(25, 16)
#define TMG_REG__vbp_WORD	6
#define TMG_REG__vbp_MASK	GENMASK(11, 0)
#define TMG_REG__h_active_WORD	6
#define TMG_REG__h_active_MASK	GENMASK(29, 16)
#define TMG_REG__v_active_WORD	7
#define TMG_REG__v_active_MASK	GENMASK(13, 0)
#define TMG_REG__background_r_WORD	9
#define TMG_REG__background_r_MASK	GENMASK(11, 0)
#define TMG_REG__background_g_WORD	9
#define TMG_REG__background_g_MASK	GENMASK(27, 16)
#define TMG_REG__background_b_WORD	10
#define TMG_REG__background_b_MASK	GENMASK(11, 0)
#define TMG_REG__eof_1st_ln_dly_num_WORD	11
#define TMG_REG__eof_1st_ln_dly_num_MASK	GENMASK(15, 0)
#define TMG_REG__eof_2nd_ln_dly_num_WORD	11
#define TMG_REG__eof_2nd_ln_dly_num_MASK	GENMASK(31, 16)
#define TMG_REG__sof_pre_ln_num_WORD	14
#define TMG_REG__sof_pre_ln_num_MASK	GENMASK(16, 0)

#endif /* SATURN_FIELDS_H */

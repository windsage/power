/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef ANDROID_UTILITY_THERMAL_H
#define ANDROID_UTILITY_THERMAL_H

#include <string>

using namespace std;

#define MAX_CONF_NAME   (19)

enum thermal_error{
    TS_ERR_NO_FILE_EXIST = -3,  /*< file is not exist */
    TS_ERR_NO_INIT = -2,        /*< not initialized */
    TS_ERR_NO_LOAD = -1,        /*< unnecessary to load conf */
    TS_ERR_UNKNOW
};

typedef struct {
    int policy_count;
    int policy_active;
} thermal_policy_manager_type;

typedef struct {
    char conf_name[MAX_CONF_NAME];
    int perf_used;
} thermal_conf_type;

struct policy_ref_t {
    unsigned int id;
    char policy_name[MAX_CONF_NAME];
    unsigned int refcnt;
    struct policy_ref_t *next;
};

struct policy_status_t {
    int active_policy;
    unsigned int max_id;
    unsigned int list_len;
    struct policy_ref_t *list;
};

extern int load_thm_api_start(int idx, void *scn);
extern int load_thm_api_stop(int idx, void *scn);
extern int reset_thermal_policy(int power_on_init);
extern int frs_set(int idx, void *scn);
extern int frs_unset(int idx, void *scn);
extern int frs_threshold(int idx, void *scn);
extern int frs_unset_threshold(int idx, void *scn);
extern int frs_target_temp(int idx, void *scn);
extern int frs_unset_target_temp(int idx, void *scn);
extern int frs_max_fps(int idx, void *scn);
extern int frs_unset_max_fps(int idx, void *scn);
extern int frs_min_fps(int idx, void *scn);
extern int frs_unset_min_fps(int idx, void *scn);
extern int frs_ceiling(int idx, void *scn);
extern int frs_unset_ceiling(int idx, void *scn);
extern int frs_ceiling_th(int idx, void *scn);
extern int frs_unset_ceiling_th(int idx, void *scn);
extern int frs_ptime(int idx, void *scn);
extern int frs_unset_ptime(int idx, void *scn);
extern int frs_ot_ptime(int idx, void *scn);
extern int frs_unset_ot_ptime(int idx, void *scn);
extern int frs_throttled(int idx, void *scn);
extern int frs_unset_throttled(int idx, void *scn);
extern int frs_ot_throttled(int idx, void *scn);
extern int frs_unset_ot_throttled(int idx, void *scn);
extern int frs_max_throttled_fps(int idx, void *scn);
extern int frs_unset_max_throttled_fps(int idx, void *scn);
extern int frs_fps_ratio(int idx, void *scn);
extern int frs_unset_fps_ratio(int idx, void *scn);
extern int frs_fps_ratio_step(int idx, void *scn);
extern int frs_unset_fps_ratio_step(int idx, void *scn);
extern int frs_mode(int idx, void *scn);
extern int frs_unset_mode(int idx, void *scn);
extern int frs_max_fps_diff(int idx, void *scn);
extern int frs_unset_max_fps_diff(int idx, void *scn);
extern int frs_ur_ptime(int idx, void *scn);
extern int frs_unset_ur_ptime(int idx, void *scn);
extern int frs_urgent_temp_offset(int idx, void *scn);
extern int frs_unset_urgent_temp_offset(int idx, void *scn);
extern int frs_tz_idx(int idx, void *scn);
extern int frs_unset_tz_idx(int idx, void *scn);
extern int frs_sys_ceiling(int idx, void *scn);
extern int frs_unset_sys_ceiling(int idx, void *scn);
extern int frs_cold_threshold(int idx, void *scn);
extern int frs_unset_cold_threshold(int idx, void *scn);
extern int frs_limit_cfreq(int idx, void *scn);
extern int frs_unset_limit_cfreq(int idx, void *scn);
extern int frs_limit_rfreq(int idx, void *scn);
extern int frs_unset_limit_rfreq(int idx, void *scn);
extern int frs_limit_cfreq_m(int idx, void *scn);
extern int frs_unset_limit_cfreq_m(int idx, void *scn);
extern int frs_limit_rfreq_m(int idx, void *scn);
extern int frs_unset_limit_rfreq_m(int idx, void *scn);
extern int frs_ct_ptime(int idx, void *scn);
extern int frs_unset_ct_ptime(int idx, void *scn);
extern int frs_min_fps_target_temp(int idx, void *scn);
extern int frs_unset_min_fps_target_temp(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_1(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_1(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_2(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_2(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_3(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_3(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_4(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_4(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_5(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_5(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_6(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_6(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_7(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_7(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster0_8(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster0_8(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_1(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_1(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_2(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_2(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_3(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_3(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_4(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_4(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_5(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_5(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_6(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_6(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_7(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_7(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster1_8(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster1_8(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_1(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_1(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_2(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_2(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_3(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_3(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_4(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_4(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_5(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_5(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_6(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_6(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_7(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_7(int idx, void *scn);
extern int frs_freq_temp_mapping_cluster2_8(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_cluster2_8(int idx, void *scn);
extern int frs_freq_temp_mapping_hysteresis(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_hysteresis(int idx, void *scn);
extern int frs_freq_temp_mapping_ptime(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_ptime(int idx, void *scn);
extern int frs_freq_temp_mapping_tz_idx(int idx, void *scn);
extern int frs_unset_freq_temp_mapping_tz_idx(int idx, void *scn);
extern int frs_set_reset(int idx, void *scn);
extern int frs_unset_reset(int idx, void *scn);

//BSP:add for thermal policy switch by jian.li at 20240424 start
extern int change_thm_policy_by_ambient_temperature(int threshold_with_thm_idx, void *scn);
extern int unchange_thm_policy_by_ambient_temperature(int threshold_with_thm_idx, void *scn);
extern int change_current_ambient_temperature(int temperature, void *scn);
extern int unchange_current_ambient_temperature(int temperature, void *scn);
extern int change_thm_policy_by_game_mode(int game_mode_with_thm_idx, void *scn);
extern int unchange_thm_policy_by_game_mode(int game_mode_with_thm_idx, void *scn);
extern int set_game_mode(int game_mode, void *scn);
extern int unset_game_mode(int game_mode, void *scn);
extern int set_fold_state(int fold_state, void *scn);
extern int unset_fold_state(int fold_state, void *scn);
extern int change_thm_policy_by_folded(int fold_state, void *scn);
extern int unchange_thm_policy_by_folded(int fold_state, void *scn);
extern int set_multiwindow_state(int multiwindow_state, void *scn);
extern int unset_multiwindow_state(int multiwindow_state, void *scn);
extern int close_thm_policy_by_engineer_mode(int close_state, void *scn);
extern int unclose_thm_policy_by_engineer_mode(int close_state, void *scn);
//BSP:add for thermal policy switch by jian.li at 20240424 end
//BSP:add for mbrain by guodong.zhang 20241223 start
extern int acquire_mbrain_data(int mbrain_data_state, void *scn);
extern int unacquire_mbrain_data(int mbrain_data_state, void *scn);
extern int open_mbrain_switch(int mbrain_state, void *scn);
extern int close_mbrain_switch(int mbrain_state, void *scn);
extern int copy_mbrain_file(int mbrain_copy, void *scn);
extern int uncopy_mbrain_file(int mbrain_copy, void *scn);
//BSP:add for mbrain by guodong.zhang 20241223 end

#endif // ANDROID_UTILITY_THERMAL_H


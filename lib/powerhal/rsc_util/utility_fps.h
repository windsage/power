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

#ifndef ANDROID_UTILITY_FPS_H
#define ANDROID_UTILITY_FPS_H

#define PATH_FSTB_FPS             "/sys/kernel/fpsgo/fstb/fstb_level"
#define PATH_FSTB_SOFT_FPS        "/sys/kernel/fpsgo/fstb/fstb_soft_level"
#define PATH_FSTB_LIST            "/sys/kernel/fpsgo/fstb/fstb_fps_list"
#define PATH_FSTB_LIST_FILE       "/vendor/etc/fstb.cfg"
#define PATH_FSTB_LIST_FILE_2     "/data/vendor/powerhal/fstb.cfg"
#define PATH_GBE_LIST             "/sys/kernel/gbe/gbe_boost_list1"
#define PATH_GBE_LIST_FILE        "/vendor/etc/gbe.cfg"
#define PATH_XGF_LIST             "/sys/kernel/fpsgo/xgf/xgf_spid_list"
#define PATH_XGF_LIST_FILE        "/vendor/etc/xgf.cfg"
#define PATH_FPSGO_CONTROL_BY_PID        "/sys/kernel/fpsgo/composer/fpsgo_control_pid"
#define PATH_FPSGO_EMA2_ENABLE_BY_PID    "/sys/kernel/fpsgo/xgf/xgf_ema2_enable_by_pid"
#define PATH_FPSGO_FILTER_DEP_TASK_ENABLE_BY_PID    "/sys/kernel/fpsgo/xgf/xgf_filter_dep_task_enable_by_pid"
#define PATH_FSTBV2_ENABLE_BY_PID        "/sys/kernel/fpsgo/fstb/fstb_self_ctrl_fps_enable_by_pid"
#define PATH_RESCUE_SECOND_ENABLE_BY_PID "/sys/kernel/fpsgo/fbt/fbt_attr_by_pid"
#define PATH_FSTB_NOTIFY_FPS_BY_PID      "/sys/kernel/fpsgo/fstb/notify_fstb_target_fps_by_pid"

#define DECLARE_FBT_SET_ATTR_BY_PID(name) \
extern int set_##name##_by_pid(int value, void *scn);

#define DECLARE_FBT_UNSET_ATTR_BY_PID(name) \
extern int unset_##name##_by_pid(int value, void *scn);

extern int fstb_init(int power_on_init);
extern int setFstbFpsHigh(int fps_high, void *scn);
extern int setFstbFpsLow(int fps_low, void *scn);
extern int setFstbSoftFpsHigh(int fps_high, void *scn);
extern int setFstbSoftFpsLow(int fps_low, void *scn);
extern int notify_targetFPS_pid(int pid, void *scn);
extern int fpsgo_ctrl_render_thread(int rtid, void *scn);
extern int fpsgo_set_video_pid_thread(int rtid, void *scn);
extern int fpsgo_clear_video_pid_thread(int rtid, void *scn);
extern int fpsgo_by_pid_init(int power_on);
extern int set_fpsgo_render_pid(int value, void *scn);
extern int get_fpsgo_render_pid(void);
extern int set_fpsgo_unsupported_func(int value, void *scn);
extern int unset_fpsgo_unsupported_func(int value, void *scn);
extern int set_fpsgo_control_by_pid(int value, void *scn);
extern int unset_fpsgo_control_by_pid(int value, void *scn);
extern int set_fpsgo_ema2_enable_by_pid(int value, void *scn);
extern int unset_fpsgo_ema2_enable_by_pid(int value, void *scn);
extern int set_fpsgo_filter_dep_task_enable_by_pid(int value, void *scn);
extern int unset_fpsgo_filter_dep_task_enable_by_pid(int value, void *scn);
extern int set_fstbv2_enable_by_pid(int value, void *scn);
extern int unset_fstbv2_enable_by_pid(int value, void *scn);
extern int set_notify_fps_by_pid(int value, void *scn);
extern int unset_notify_fps_by_pid(int value, void *scn);
extern int set_bypass_nonsf_by_pid(int value, void *scn);
extern int unset_bypass_nonsf_by_pid(int value, void *scn);
extern int set_control_api_mask_by_pid(int value, void *scn);
extern int unset_control_api_mask_by_pid(int value, void *scn);
extern int set_control_hwui_by_pid(int value, void *scn);
extern int unset_control_hwui_by_pid(int value, void *scn);
int set_app_cam_fps_align_margin_by_pid(int value, void *scn);
int unset_app_cam_fps_align_margin_by_pid(int value, void *scn);
int set_app_cam_time_align_ratio_by_pid(int value, void *scn);
int unset_app_cam_time_align_ratio_by_pid(int value, void *scn);
int set_jank_detection_mask(int value, void *scn);
int unset_jank_detection_mask(int value, void *scn);
int set_dep_loading_thr_by_pid(int value, void *scn);
int unset_dep_loading_thr_by_pid(int value, void *scn);
int set_cam_bypass_window_ms_by_pid(int value, void *scn);
int unset_cam_bypass_window_ms_by_pid(int value, void *scn);

DECLARE_FBT_SET_ATTR_BY_PID(rescue_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(rescue_enable)

DECLARE_FBT_SET_ATTR_BY_PID(rescue_second_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(rescue_second_enable)

DECLARE_FBT_SET_ATTR_BY_PID(loading_th)
DECLARE_FBT_UNSET_ATTR_BY_PID(loading_th)

DECLARE_FBT_SET_ATTR_BY_PID(light_loading_policy)
DECLARE_FBT_UNSET_ATTR_BY_PID(light_loading_policy)

DECLARE_FBT_SET_ATTR_BY_PID(llf_task_policy)
DECLARE_FBT_UNSET_ATTR_BY_PID(llf_task_policy)

DECLARE_FBT_SET_ATTR_BY_PID(filter_frame_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(filter_frame_enable)

DECLARE_FBT_SET_ATTR_BY_PID(separate_aa)
DECLARE_FBT_UNSET_ATTR_BY_PID(separate_aa)

DECLARE_FBT_SET_ATTR_BY_PID(separate_release_sec)
DECLARE_FBT_UNSET_ATTR_BY_PID(separate_release_sec)

DECLARE_FBT_SET_ATTR_BY_PID(boost_affinity)
DECLARE_FBT_UNSET_ATTR_BY_PID(boost_affinity)

DECLARE_FBT_SET_ATTR_BY_PID(boost_lr)
DECLARE_FBT_UNSET_ATTR_BY_PID(boost_lr)

DECLARE_FBT_SET_ATTR_BY_PID(cpumask_heavy)
DECLARE_FBT_UNSET_ATTR_BY_PID(cpumask_heavy)

DECLARE_FBT_SET_ATTR_BY_PID(cpumask_second)
DECLARE_FBT_UNSET_ATTR_BY_PID(cpumask_second)

DECLARE_FBT_SET_ATTR_BY_PID(cpumask_others)
DECLARE_FBT_UNSET_ATTR_BY_PID(cpumask_others)

DECLARE_FBT_SET_ATTR_BY_PID(limit_uclamp)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_uclamp)

DECLARE_FBT_SET_ATTR_BY_PID(limit_ruclamp)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_ruclamp)

DECLARE_FBT_SET_ATTR_BY_PID(limit_uclamp_m)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_uclamp_m)

DECLARE_FBT_SET_ATTR_BY_PID(limit_ruclamp_m)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_ruclamp_m)

DECLARE_FBT_SET_ATTR_BY_PID(qr_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(qr_enable)

DECLARE_FBT_SET_ATTR_BY_PID(qr_t2wnt_x)
DECLARE_FBT_UNSET_ATTR_BY_PID(qr_t2wnt_x)

DECLARE_FBT_SET_ATTR_BY_PID(qr_t2wnt_y_p)
DECLARE_FBT_UNSET_ATTR_BY_PID(qr_t2wnt_y_p)

DECLARE_FBT_SET_ATTR_BY_PID(qr_t2wnt_y_n)
DECLARE_FBT_UNSET_ATTR_BY_PID(qr_t2wnt_y_n)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_enable)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_fps_margin)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_fps_margin)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_up_sec_pct)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_up_sec_pct)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_down_sec_pct)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_down_sec_pct)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_down_step)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_down_step)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_reserved_up_quota_pct)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_reserved_up_quota_pct)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_up_step)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_up_step)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_reserved_down_quota_pct)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_reserved_down_quota_pct)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_enq_bound_thrs)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_enq_bound_thrs)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_deq_bound_thrs)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_deq_bound_thrs)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_enq_bound_quota)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_enq_bound_quota)

DECLARE_FBT_SET_ATTR_BY_PID(gcc_deq_bound_quota)
DECLARE_FBT_UNSET_ATTR_BY_PID(gcc_deq_bound_quota)

DECLARE_FBT_SET_ATTR_BY_PID(separate_pct_m)
DECLARE_FBT_UNSET_ATTR_BY_PID(separate_pct_m)

DECLARE_FBT_SET_ATTR_BY_PID(separate_pct_b)
DECLARE_FBT_UNSET_ATTR_BY_PID(separate_pct_b)

DECLARE_FBT_SET_ATTR_BY_PID(separate_pct_other)
DECLARE_FBT_UNSET_ATTR_BY_PID(separate_pct_other)

DECLARE_FBT_SET_ATTR_BY_PID(blc_boost)
DECLARE_FBT_UNSET_ATTR_BY_PID(blc_boost)

DECLARE_FBT_SET_ATTR_BY_PID(limit_cfreq2cap)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_cfreq2cap)

DECLARE_FBT_SET_ATTR_BY_PID(limit_rfreq2cap)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_rfreq2cap)

DECLARE_FBT_SET_ATTR_BY_PID(limit_cfreq2cap_m)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_cfreq2cap_m)

DECLARE_FBT_SET_ATTR_BY_PID(limit_rfreq2cap_m)
DECLARE_FBT_UNSET_ATTR_BY_PID(limit_rfreq2cap_m)

DECLARE_FBT_SET_ATTR_BY_PID(target_time_up_bound)
DECLARE_FBT_UNSET_ATTR_BY_PID(target_time_up_bound)

DECLARE_FBT_SET_ATTR_BY_PID(boost_VIP)
DECLARE_FBT_UNSET_ATTR_BY_PID(boost_VIP)

DECLARE_FBT_SET_ATTR_BY_PID(group_by_lr)
DECLARE_FBT_UNSET_ATTR_BY_PID(group_by_lr)

DECLARE_FBT_SET_ATTR_BY_PID(heavy_group_num)
DECLARE_FBT_UNSET_ATTR_BY_PID(heavy_group_num)

DECLARE_FBT_SET_ATTR_BY_PID(second_group_num)
DECLARE_FBT_UNSET_ATTR_BY_PID(second_group_num)

DECLARE_FBT_SET_ATTR_BY_PID(set_ls)
DECLARE_FBT_UNSET_ATTR_BY_PID(set_ls)

DECLARE_FBT_SET_ATTR_BY_PID(ls_groupmask)
DECLARE_FBT_UNSET_ATTR_BY_PID(ls_groupmask)

DECLARE_FBT_SET_ATTR_BY_PID(vip_mask)
DECLARE_FBT_UNSET_ATTR_BY_PID(vip_mask)

DECLARE_FBT_SET_ATTR_BY_PID(set_vvip)
DECLARE_FBT_UNSET_ATTR_BY_PID(set_vvip)

DECLARE_FBT_SET_ATTR_BY_PID(vip_throttle)
DECLARE_FBT_UNSET_ATTR_BY_PID(vip_throttle)

DECLARE_FBT_SET_ATTR_BY_PID(bm_th)
DECLARE_FBT_UNSET_ATTR_BY_PID(bm_th)

DECLARE_FBT_SET_ATTR_BY_PID(ml_th)
DECLARE_FBT_UNSET_ATTR_BY_PID(ml_th)

DECLARE_FBT_SET_ATTR_BY_PID(tp_policy)
DECLARE_FBT_UNSET_ATTR_BY_PID(tp_policy)

DECLARE_FBT_SET_ATTR_BY_PID(tp_strict_middle)
DECLARE_FBT_UNSET_ATTR_BY_PID(tp_strict_middle)

DECLARE_FBT_SET_ATTR_BY_PID(tp_strict_little)
DECLARE_FBT_UNSET_ATTR_BY_PID(tp_strict_little)

DECLARE_FBT_SET_ATTR_BY_PID(gh_prefer)
DECLARE_FBT_UNSET_ATTR_BY_PID(gh_prefer)

DECLARE_FBT_SET_ATTR_BY_PID(reset_taskmask)
DECLARE_FBT_UNSET_ATTR_BY_PID(reset_taskmask)

DECLARE_FBT_SET_ATTR_BY_PID(check_buffer_quota)
DECLARE_FBT_UNSET_ATTR_BY_PID(check_buffer_quota)

DECLARE_FBT_SET_ATTR_BY_PID(expected_fps_margin)
DECLARE_FBT_UNSET_ATTR_BY_PID(expected_fps_margin)

DECLARE_FBT_SET_ATTR_BY_PID(aa_b_minus_idle_t)
DECLARE_FBT_UNSET_ATTR_BY_PID(aa_b_minus_idle_t)

DECLARE_FBT_SET_ATTR_BY_PID(quota_v2_diff_clamp_min)
DECLARE_FBT_UNSET_ATTR_BY_PID(quota_v2_diff_clamp_min)

DECLARE_FBT_SET_ATTR_BY_PID(quota_v2_diff_clamp_max)
DECLARE_FBT_UNSET_ATTR_BY_PID(quota_v2_diff_clamp_max)

DECLARE_FBT_SET_ATTR_BY_PID(rl_l2q_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(rl_l2q_enable)

DECLARE_FBT_SET_ATTR_BY_PID(rl_l2q_exp_us)
DECLARE_FBT_UNSET_ATTR_BY_PID(rl_l2q_exp_us)

DECLARE_FBT_SET_ATTR_BY_PID(powerRL_enable)
DECLARE_FBT_UNSET_ATTR_BY_PID(powerRL_enable)

DECLARE_FBT_SET_ATTR_BY_PID(powerRL_FPS_margin)
DECLARE_FBT_UNSET_ATTR_BY_PID(powerRL_FPS_margin)

DECLARE_FBT_SET_ATTR_BY_PID(powerRL_cap_limit_range)
DECLARE_FBT_UNSET_ATTR_BY_PID(powerRL_cap_limit_range)

#endif // ANDROID_UTILITY_FPS_H


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

#ifndef ANDROID_UTILITY_SCHED_H
#define ANDROID_UTILITY_SCHED_H


#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

#define PATH_EAS_IOCTL "/proc/easmgr/eas_ioctl"
#define PATH_CPUQOS_IOCTL "/proc/cpuqosmgr/cpuqos_ioctl"
#define PATH_CORE_CTL_IOCTL "/proc/cpumgr/core_ioctl"
#define PATH_SYSCTL_PELT_MULTIPLIER "/proc/sys/kernel/sched_pelt_multiplier"

struct _CORE_CTL_PACKAGE {
	union {
		__u32 cid;
		__u32 cpu;
	};
	union {
		__u32 min;
		__u32 is_pause;
		__u32 throttle_ms;
		__u32 not_preferred_cpus;
		__u32 boost;
		__u32 thres;
		__u32 enable_policy;
	};
	__u32 max;
};

struct gas_ctrl {
	int val;
	int force_ctrl;
};

struct gas_thr {
	int grp_id;
	int val;
};

struct gas_margin_thr {
	int gear_id;
	int group_id;
	int margin;
	int converge_thr;
};

struct _CPUQOS_V3_PACKAGE {
		__u32 mode;
		__u32 pid;
		__u32 set_task;
		__u32 group_id;
		__u32 set_group;
		__u32 user_pid;
		__u32 bitmask;
		__u32 set_user_group;
};

#define EAS_SYNC_SET             			    _IOW('g', 1, unsigned int)
#define EAS_SYNC_GET           				    _IOW('g', 2, unsigned int)
#define EAS_PERTASK_LS_SET     				    _IOW('g', 3, unsigned int)
#define EAS_PERTASK_LS_GET    				    _IOR('g', 4, unsigned int)
#define EAS_ACTIVE_MASK_GET          			_IOR('g', 5, unsigned int)
#define EAS_NEWLY_IDLE_BALANCE_INTERVAL_SET		_IOW('g', 6,  unsigned int)
#define EAS_NEWLY_IDLE_BALANCE_INTERVAL_GET		_IOR('g', 7,  unsigned int)
#define EAS_GET_THERMAL_HEADROOM_INTERVAL_SET	_IOW('g', 8,  unsigned int)
#define EAS_GET_THERMAL_HEADROOM_INTERVAL_GET	_IOR('g', 9,  unsigned int)
#define EAS_SET_SYSTEM_MASK						_IOW('g', 10,  unsigned int)
#define EAS_GET_SYSTEM_MASK						_IOW('g', 11,  unsigned int)
#define EAS_SBB_ALL_SET				_IOW('g', 12,  unsigned int)
#define EAS_SBB_ALL_UNSET			_IOW('g', 13,  unsigned int)
#define EAS_SBB_GROUP_SET			_IOW('g', 14,  unsigned int)
#define EAS_SBB_GROUP_UNSET			_IOW('g', 15,  unsigned int)
#define EAS_SBB_TASK_SET			_IOW('g', 16,  unsigned int)
#define EAS_SBB_TASK_UNSET			_IOW('g', 17,  unsigned int)
#define EAS_SBB_ACTIVE_RATIO		_IOW('g', 18,  unsigned int)
#define EAS_TASK_IDLE_PREFER_SET    _IOW('g', 19,  int)
#define EAS_SCHED_CTL_EST           _IOW('g', 20,  int)
#define EAS_SCHED_CTL_TARGET_FREQ_C0    _IOW('g', 21,  int)
#define EAS_SCHED_CTL_TARGET_MARGIN_C0 _IOW('g', 22,  int)
#define EAS_SCHED_CTL_TARGET_FREQ_C1    _IOW('g', 23,  int)
#define EAS_SCHED_CTL_TARGET_MARGIN_C1 _IOW('g', 24,  int)
#define EAS_SCHED_CTL_TARGET_FREQ_C2    _IOW('g', 25,  int)
#define EAS_SCHED_CTL_TARGET_MARGIN_C2 _IOW('g', 26,  int)
#define EAS_SET_CPUMASK_TA		    _IOW('g', 27,  unsigned int)
#define EAS_SET_CPUMASK_BACKGROUND	_IOW('g', 28,  unsigned int)
#define EAS_SET_CPUMASK_FOREGROUND	_IOW('g', 29,  unsigned int)
#define EAS_SET_TASK_LS			    _IOW('g', 30,  int)
#define EAS_UNSET_TASK_LS		    _IOW('g', 31,  int)
#define EAS_IGNORE_IDLE_UTIL_CTRL    _IOW('g', 33,  unsigned int)

#define EAS_SET_TASK_VIP			_IOW('g', 34,  unsigned int)
#define EAS_UNSET_TASK_VIP			_IOW('g', 35,  unsigned int)
#define EAS_SET_TA_VIP				_IOW('g', 36,  unsigned int)
#define EAS_UNSET_TA_VIP			_IOW('g', 37,  unsigned int)
#define EAS_SET_FG_VIP				_IOW('g', 38,  unsigned int)
#define EAS_UNSET_FG_VIP			_IOW('g', 39,  unsigned int)
#define EAS_SET_BG_VIP				_IOW('g', 40,  unsigned int)
#define EAS_UNSET_BG_VIP			_IOW('g', 41,  unsigned int)
#define EAS_SET_LS_VIP				_IOW('g', 42,  unsigned int)
#define EAS_UNSET_LS_VIP			_IOW('g', 43,  unsigned int)

#define EAS_GEAR_MIGR_DN_PCT		_IOW('g', 44,  unsigned int)
#define EAS_GEAR_MIGR_UP_PCT		_IOW('g', 45,  unsigned int)
#define EAS_GEAR_MIGR_SET			_IOW('g', 46,  unsigned int)
#define EAS_GEAR_MIGR_UNSET			_IOW('g', 47,  unsigned int)

#define EAS_TASK_GEAR_HINTS_START	_IOW('g', 48,  unsigned int)
#define EAS_TASK_GEAR_HINTS_NUM		_IOW('g', 49,  unsigned int)
#define EAS_TASK_GEAR_HINTS_REVERSE	_IOW('g', 50,  unsigned int)
#define EAS_TASK_GEAR_HINTS_SET		_IOW('g', 51,  unsigned int)
#define EAS_TASK_GEAR_HINTS_UNSET	_IOW('g', 52,  unsigned int)

#define EAS_SET_GAS_CTRL			_IOW('g', 53,  struct gas_ctrl)
#define EAS_SET_GAS_THR				_IOW('g', 54,  struct gas_thr)
#define EAS_RESET_GAS_THR			_IOW('g', 55,  int)
#define EAS_SET_GAS_MARG_THR		_IOW('g', 56,  struct gas_margin_thr)
#define EAS_RESET_GAS_MARG_THR		_IOW('g', 57,  int)

#define EAS_DPT_CTRL                _IOW('g', 60,  int)
#define EAS_RUNNABLE_BOOST_SET      _IOW('g', 61,  unsigned int)
#define EAS_RUNNABLE_BOOST_UNSET    _IOW('g', 62,  unsigned int)
#define EAS_SET_DSU_IDLE			_IOW('g', 63,  unsigned int)

#define CORE_CTL_FORCE_RESUME_CPU               _IOW('g', 1,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_FORCE_PAUSE_CPU                _IOW('g', 2,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_OFFLINE_THROTTLE_MS        _IOW('g', 3,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_LIMIT_CPUS                 _IOW('g', 4,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_NOT_PREFERRED              _IOW('g', 5,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_BOOST                      _IOW('g', 6,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_UP_THRES                   _IOW('g', 7,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_ENABLE_POLICY                  _IOW('g', 8,  struct _CORE_CTL_PACKAGE)
#define CORE_CTL_SET_CPU_BUSY_THRES             _IOW('g', 9,  struct _CORE_CTL_PACKAGE)

#define CPUQOS_V3_SET_CPUQOS_MODE				_IOW('g', 14, struct _CPUQOS_V3_PACKAGE)
#define CPUQOS_V3_SET_CT_TASK					_IOW('g', 15, struct _CPUQOS_V3_PACKAGE)
#define CPUQOS_V3_SET_CT_GROUP					_IOW('g', 16, struct _CPUQOS_V3_PACKAGE)
#define CPUQOS_V3_SET_TASK_TO_USER_GROUP		_IOW('g', 17, struct _CPUQOS_V3_PACKAGE)
#define CPUQOS_V3_SET_CCL_TO_USER_GROUP			_IOW('g', 18, struct _CPUQOS_V3_PACKAGE)

#define MAX_CPU_NUM 16

#define CPUCTL_GROUP_ROOT	0
#define CPUCTL_GROUP_FG		1
#define CPUCTL_GROUP_BG	    2
#define CPUCTL_GROUP_TA		3
#define CPUCTL_GROUP_RT		4
#define CPUCTL_GROUP_SYS	5
#define CPUCTL_GROUP_SYSBG	6

#define DECLARE_SET_SCHED(class, name) \
extern int set_##class##_##name(int value, void *scn);

#define DECLARE_UNSET_SCHED(class, name) \
extern int unset_##class##_##name(int value, void *scn);

#define DECLARE_SET_CPUQOS_USER_GROUP(class) \
extern int set_cpuqos_user_group_##class(int value, void *scn);

#define DECLARE_UNSET_CPUQOS_USER_GROUP(class) \
extern int unset_cpuqos_user_group_##class(int value, void *scn);

#define DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(class) \
extern int set_cpuqos_ccl_to_user_group_##class(int value, void *scn);

#define DECLARE_SET_CPU_ISO(cpu) \
extern int set_force_cpu_##cpu##_isolated(int value, void *scn);

enum cpuqos_perf_mode {
	AGGRESSIVE = 0,
	BALANCE,  //default
	CONSERVATIVE,
	DISABLE
};

extern int set_cpu_boost(int value, void *scn);
extern int set_force_cpu_isolated(int value, void *scn);
extern int unset_force_cpu_isolated(int value, void *scn);
extern int set_offline_throttle_cluster0(int value, void *scn);
extern int set_offline_throttle_cluster1(int value, void *scn);
extern int set_offline_throttle_cluster2(int value, void *scn);
extern int set_not_preferred(int value, void *scn);
extern int core_ctl_set_cpu_busy_thres_cluster0(int value, void *scn);
extern int core_ctl_set_cpu_busy_thres_cluster1(int value, void *scn);
extern int core_ctl_set_cpu_busy_thres_cluster2(int value, void *scn);
extern int set_btask_up_thres_cluster0(int value, void *scn);
extern int set_btask_up_thres_cluster1(int value, void *scn);
extern int set_top_grp_threshold(int value, void *scn);
extern int reset_top_grp_threshold(int value, void *scn);
extern int enable_top_grp_aware(int value, void *scn);
extern int set_cpuqos_mode(int value, void *scn);
extern int set_background_ct(int value, void *scn);
extern int set_system_background_ct(int value, void *scn);
extern int set_topapp_ct(int value, void *scn);
extern int set_root_ct(int value, void *scn);
extern int set_rt_ct(int value, void *scn);
extern int set_foreground_ct(int value, void *scn);
extern int set_system_ct(int value, void *scn);
extern int set_per_task_ct(int value, void *scn);
extern int unset_per_task_ct(int value, void *scn);
extern int sched_set_pelt_halflife(int value, void *scn);
extern int sched_unset_pelt_halflife(int value, void *scn);
extern int sched_set_pelt_halflife_high_priority(int value, void *scn);
extern int sched_unset_pelt_halflife_high_priority(int value, void *scn);
extern int sched_set_update_pelt_throttle_ms(int value, void *scn);

DECLARE_SET_CPU_ISO(0)
DECLARE_SET_CPU_ISO(1)
DECLARE_SET_CPU_ISO(2)
DECLARE_SET_CPU_ISO(3)
DECLARE_SET_CPU_ISO(4)
DECLARE_SET_CPU_ISO(5)
DECLARE_SET_CPU_ISO(6)
DECLARE_SET_CPU_ISO(7)

DECLARE_SET_SCHED(sync, flag)
DECLARE_SET_SCHED(per_task, prefer_idle)
DECLARE_SET_SCHED(newly, idle_balance_interval_us)
DECLARE_SET_SCHED(sbb, all)
DECLARE_UNSET_SCHED(sbb, all)
DECLARE_SET_SCHED(sbb, group)
DECLARE_UNSET_SCHED(sbb, group)
DECLARE_SET_SCHED(sbb, task)
DECLARE_UNSET_SCHED(sbb, task)
DECLARE_SET_SCHED(sbb, active_ratio)
DECLARE_SET_SCHED(thermal, headroom_interval_tick)
DECLARE_SET_SCHED(task, idle_prefer)
DECLARE_SET_SCHED(sched, ctl_util_est)
DECLARE_SET_SCHED(sched, ctl_target_freq_c0)
DECLARE_SET_SCHED(sched, ctl_target_margin_c0)
DECLARE_SET_SCHED(sched, ctl_target_freq_c1)
DECLARE_SET_SCHED(sched, ctl_target_margin_c1)
DECLARE_SET_SCHED(sched, ctl_target_freq_c2)
DECLARE_SET_SCHED(sched, ctl_target_margin_c2)

DECLARE_SET_SCHED(system, cpumask_int)

DECLARE_SET_SCHED(sched, ta_mask)
DECLARE_SET_SCHED(sched, fg_mask)
DECLARE_SET_SCHED(sched, bg_mask)
DECLARE_SET_SCHED(sched, task_ls)
DECLARE_UNSET_SCHED(sched, task_ls)

DECLARE_SET_SCHED(sched, ignore_idle_util_ctrl)

DECLARE_SET_SCHED(sched, task_vip)
DECLARE_UNSET_SCHED(sched, task_vip)
DECLARE_SET_SCHED(sched, ta_vip)
DECLARE_UNSET_SCHED(sched, ta_vip)
DECLARE_SET_SCHED(sched, fg_vip)
DECLARE_UNSET_SCHED(sched, fg_vip)
DECLARE_SET_SCHED(sched, bg_vip)
DECLARE_UNSET_SCHED(sched, bg_vip)
DECLARE_SET_SCHED(sched, ls_vip)
DECLARE_UNSET_SCHED(sched, ls_vip)

DECLARE_SET_SCHED(sched, gear_migr_dn_pct)
DECLARE_SET_SCHED(sched, gear_migr_up_pct)
DECLARE_SET_SCHED(sched, gear_migr_cid)
DECLARE_UNSET_SCHED(sched, gear_migr_cid)

DECLARE_SET_SCHED(sched, task_gear_hints_start)
DECLARE_SET_SCHED(sched, task_gear_hints_num)
DECLARE_SET_SCHED(sched, task_gear_hints_reverse)
DECLARE_SET_SCHED(sched, task_gear_hints_pid)
DECLARE_UNSET_SCHED(sched, task_gear_hints_pid)
DECLARE_SET_SCHED(sched, dpt_ctrl)
DECLARE_SET_SCHED(sched, runnable_boost)
DECLARE_UNSET_SCHED(sched, runnable_boost)
DECLARE_SET_SCHED(sched, dsu_idle)

DECLARE_SET_CPUQOS_USER_GROUP(1)
DECLARE_SET_CPUQOS_USER_GROUP(2)
DECLARE_SET_CPUQOS_USER_GROUP(3)
DECLARE_SET_CPUQOS_USER_GROUP(4)
DECLARE_SET_CPUQOS_USER_GROUP(5)
DECLARE_SET_CPUQOS_USER_GROUP(6)
DECLARE_UNSET_CPUQOS_USER_GROUP(1)
DECLARE_UNSET_CPUQOS_USER_GROUP(2)
DECLARE_UNSET_CPUQOS_USER_GROUP(3)
DECLARE_UNSET_CPUQOS_USER_GROUP(4)
DECLARE_UNSET_CPUQOS_USER_GROUP(5)
DECLARE_UNSET_CPUQOS_USER_GROUP(6)

DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(1)
DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(2)
DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(3)
DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(4)
DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(5)
DECLARE_SET_CPUQOS_CCL_T0_USER_GROUP(6)

#endif

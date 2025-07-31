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

#define LOG_TAG "libPowerHal"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "common.h"
#include "utility_sched.h"
#include "perfservice.h"
#include "mtkperf_resource.h"


static int eas_fd = -1;
static int core_ctl_fd = -1;
static int cpuqos_fd = -1;
static int cpu_num = get_cpu_num();
static int isolation_cpu_count[MAX_CPU_NUM] = {0};
static int isolation_cpu_max = (1 << cpu_num) - 1;
static int pelt_halflife_value = -1;
static int pelt_halflife_value_high_priority = -1;

#define SCHED_SET_BY_IOCTL(class, name, cmd) \
int set_##class##_##name(int value, void *scn) { \
    int ret;  \
    LOG_D("%d, %p", value, scn); \
    if (check_eas_valid() == 0) {  \
        ret = ioctl(eas_fd, cmd, &value);  \
        LOG_D("ret %d", ret);  \
    }  \
    return 0;  \
}

#define SCHED_UNSET_BY_IOCTL(class, name, cmd) \
int unset_##class##_##name(int value, void *scn) { \
    int ret;  \
    LOG_D("%d, %p", value, scn); \
    if (check_eas_valid() == 0) {  \
        ret = ioctl(eas_fd, cmd, &value);  \
        LOG_D("ret %d", ret);  \
    }  \
    return 0;  \
}

#define SCHED_SET_MASK_BY_IOCTL(class, name, mask_cmd, eas_cmd) \
int set_##class##_##name(int value, void *scn) { \
    int ret;  \
    value = get_system_mask_status(mask_cmd); \
    if (value == 0) { /* no one sets cpu mask, reset to 0xFF */ \
        value = (1 << cpu_num) - 1; \
    } \
    LOG_D("%d (0x%x)", value, value); \
    if (check_eas_valid() == 0) {  \
        ret = ioctl(eas_fd, eas_cmd, &value);  \
        LOG_D("ret %d", ret);  \
    }  \
    return 0;  \
}

int check_eas_valid()
{
    if (eas_fd >= 0) {
        return 0;
    } else if (eas_fd == -1) {
        eas_fd = open(PATH_EAS_IOCTL, O_RDONLY);
        if (eas_fd < 0 && errno == ENOENT) {
            eas_fd = -2;
        }
        if (eas_fd == -1) {
            LOG_E("Can't open %s: %s", PATH_EAS_IOCTL, strerror(errno));
            return -1;
        }
    } else if (eas_fd == -2) {
        return -2;
    }
    return 0;
}

int check_corectl_valid()
{
    if (core_ctl_fd >= 0) {
        return 0;
    } else if (core_ctl_fd == -1) {
        core_ctl_fd = open(PATH_CORE_CTL_IOCTL, O_RDONLY);
        if (core_ctl_fd < 0 && errno == ENOENT) {
            core_ctl_fd = -2;
        }
        if (core_ctl_fd == -1) {
            LOG_E("Can't open %s: %s", PATH_CORE_CTL_IOCTL, strerror(errno));
            return -1;
        }
    } else if (core_ctl_fd == -2) {
        return -2;
    }
    return 0;
}

int check_cpuqos_valid()
{
    if (cpuqos_fd >= 0) {
        return 0;
    } else if (cpuqos_fd == -1) {
        cpuqos_fd = open(PATH_CPUQOS_IOCTL, O_RDONLY);
        if (cpuqos_fd < 0 && errno == ENOENT) {
            cpuqos_fd = -2;
        }
        if (cpuqos_fd == -1) {
            LOG_E("Can't open %s: %s", PATH_CPUQOS_IOCTL, strerror(errno));
            return -1;
        }
    } else if (cpuqos_fd == -2) {
        return -2;
    }
    return 0;
}

int write_data_to_file(int value, char * path)
{
    char str[64];
    struct stat stat_buf;

    if (0 != stat(path, &stat_buf)) {
        LOG_D("cannot access %s", path);
        return -1;
    }

    if (snprintf(str, sizeof(str)-1, "%d", value) < 0) {
        LOG_E("snprintf error");
        return -1;
    }

    LOG_D("%d, %s", value, path);
    set_value(path, str);

    return 0;
}

// *************************************************
// de-isolate all cpu core

int set_cpu_boost(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.boost = value;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_BOOST, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

// *************************************************
// set offline throttle (unit:ms)

int set_offline_throttle_cluster0(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.throttle_ms = value;
    msg.cid = 0;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_OFFLINE_THROTTLE_MS, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int set_offline_throttle_cluster1(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.throttle_ms = value;
    msg.cid = 1;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_OFFLINE_THROTTLE_MS, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int set_offline_throttle_cluster2(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.throttle_ms = value;
    msg.cid = 2;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_OFFLINE_THROTTLE_MS, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

// *************************************************
// set big task up threshold

int set_btask_up_thres_cluster0(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.thres = value;
    msg.cid = 0;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_UP_THRES, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int set_btask_up_thres_cluster1(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.thres = value;
    msg.cid = 1;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_UP_THRES, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

// *************************************************
// set cpu_id as not preferr

int set_not_preferred(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.not_preferred_cpus = value;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_NOT_PREFERRED, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

// *************************************************
// force cpu core isolated

int set_force_cpu_isolated(int value, void *scn)
{
    char str[64];
    int i, iso_cpu_mask;

    if (check_corectl_valid() != 0)
        return -1;
    
    LOG_D("value:%d, 0x%x, scn:%p", value, value, scn);
    if (value < 0 || value >= isolation_cpu_max) {
        LOG_E("error value:%d, 0x%x", value, value);
        return -1;
    }

    isolation_cpu_count[i] = {0};
    for (i = 0; i < cpu_num; i++) {
        if ((value & (1 << i)) > 0)
            isolation_cpu_count[i]++;
    }
    iso_cpu_mask = 0;
    for (i = 0; i < cpu_num ; i++) {
        _CORE_CTL_PACKAGE msg;
        if (isolation_cpu_count[i] > 0) {
            iso_cpu_mask += (1 << i);
            msg.cpu = i;
            msg.is_pause = 1;
            ioctl(core_ctl_fd, CORE_CTL_FORCE_PAUSE_CPU, &msg);
        }
        else {
            _CORE_CTL_PACKAGE msg;
            msg.cpu = i;
            msg.is_pause = 0;
            ioctl(core_ctl_fd, CORE_CTL_FORCE_PAUSE_CPU, &msg);
        }
    }
    LOG_D("isolation_cpu_mask:0x%x", iso_cpu_mask);

    return 0;
}

int unset_force_cpu_isolated(int value, void *scn)
{
    char str[64];
    int i, iso_cpu_mask;

    if (check_corectl_valid() != 0)
        return -1;

    LOG_D("value:%d, 0x%x", value, value);
    if (value < 0 || value >= isolation_cpu_max) {
        LOG_E("error value:%d, 0x%x.", value, value);
        return -1;
    }

    for (i = 0; i < cpu_num; i++) {
        isolation_cpu_count[i] = 0;
    }
    iso_cpu_mask = 0;
    for (i = 0; i < cpu_num ; i++) {
        _CORE_CTL_PACKAGE msg;
        if (isolation_cpu_count[i] > 0) {
            iso_cpu_mask += (1 << i);
            msg.cpu = i;
            msg.is_pause = 1;
            ioctl(core_ctl_fd, CORE_CTL_FORCE_PAUSE_CPU, &msg);
        }
        else {
            _CORE_CTL_PACKAGE msg;
            msg.cpu = i;
            msg.is_pause = 0;
            ioctl(core_ctl_fd, CORE_CTL_FORCE_PAUSE_CPU, &msg);
        }
    }
    LOG_D("isolation_cpu_mask: 0x%x", iso_cpu_mask);

    return 0;
}


int set_cpuqos_mode(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.mode = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CPUQOS_MODE, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

// *************************************************
// set_ct_group(int group_id, bool set)

int set_background_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_BG;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_system_background_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_SYSBG;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_topapp_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_TA;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_root_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_ROOT;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_rt_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_RT;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_foreground_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_FG;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int set_system_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.group_id = CPUCTL_GROUP_SYS;
    msg.set_group = value;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_GROUP, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

// *************************************************
// set_ct_task(int pid, bool set)

int set_per_task_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.pid = value;
    msg.set_task = 1;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_TASK, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int unset_per_task_ct(int value, void *scn)
{
    int ret;
    _CPUQOS_V3_PACKAGE msg;
    msg.pid = value;
    msg.set_task = 0;

    LOG_D("%d, %p", value, scn);
    if (check_cpuqos_valid() == 0) {
        ret = ioctl(cpuqos_fd, CPUQOS_V3_SET_CT_TASK, &msg);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int sched_set_pelt_multiplier(int value) {
    if (value == 32) {
        value = 1;
    } else if (value == 16) {
        value = 2;
    } else {
        value = 4;
    }

    return set_value(PATH_SYSCTL_PELT_MULTIPLIER, value);
}

int sched_set_pelt_halflife(int value, void *scn)
{
    int ret;

    LOG_D("%d, %p", value, scn);
    pelt_halflife_value = value;

    if (pelt_halflife_value_high_priority == -1) {
        ret = sched_set_pelt_multiplier(value);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int sched_unset_pelt_halflife(int value, void *scn)
{
    int ret;

    LOG_D("%d, %p", value, scn);
    pelt_halflife_value = -1;

    if (pelt_halflife_value_high_priority == -1) {
        ret = sched_set_pelt_multiplier(value);
        LOG_D("ret %d", ret);
    }
    return 0;
}

int sched_set_pelt_halflife_high_priority(int value, void *scn)
{
    int ret;

    LOG_D("%d, %p", value, scn);
    pelt_halflife_value_high_priority = value;

    ret = sched_set_pelt_multiplier(value);
    LOG_D("ret %d", ret);

    return 0;
}

int sched_unset_pelt_halflife_high_priority(int value, void *scn)
{
    int ret;

    LOG_D("%d, %p", value, scn);
    pelt_halflife_value_high_priority = -1;
    if (pelt_halflife_value != -1) 
        value = pelt_halflife_value;

    ret = sched_set_pelt_multiplier(value);
    LOG_D("ret %d", ret);

    return 0;
}

int sched_set_update_pelt_throttle_ms(int value, void *scn)
{
    LOG_D("%d, %p", value, scn);
    LOG_I("Not supported.");

    return 0;
}

// *************************************************
// int core_ctl_set_cpu_busy_thres(unsigned int cid, unsigned int pct)

int core_ctl_set_cpu_busy_thres_cluster0(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.cid = 0;
    msg.thres = value;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_CPU_BUSY_THRES, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int core_ctl_set_cpu_busy_thres_cluster1(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.cid = 1;
    msg.thres = value;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_CPU_BUSY_THRES, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int core_ctl_set_cpu_busy_thres_cluster2(int value, void *scn)
{
    int ret;
    _CORE_CTL_PACKAGE msg;
    msg.cid = 2;
    msg.thres = value;

    LOG_D("%d, %p", value, scn);
    if (check_corectl_valid() == 0) {
        ret = ioctl(core_ctl_fd, CORE_CTL_SET_CPU_BUSY_THRES, &msg);
        LOG_D("ret %d", ret);
    }

    return 0;
}

int unset_sched_am_ctrl(int value, void *scn)
{
    int ret;
    value = 1;
    LOG_D("%d, %p", value, scn);
    if (check_eas_valid() == 0) {
        ret = ioctl(eas_fd, EAS_SET_AM_CTRL, &value);
        LOG_D("ret %d", ret);
    }

    return 0;
}

// *************************************************
// set sync flag
SCHED_SET_BY_IOCTL(sync, flag, EAS_SYNC_SET)

// *************************************************
// set per task prefer idle
SCHED_SET_BY_IOCTL(per_task, prefer_idle, EAS_PERTASK_LS_SET)

SCHED_SET_BY_IOCTL(newly, idle_balance_interval_us, EAS_NEWLY_IDLE_BALANCE_INTERVAL_SET)
SCHED_SET_BY_IOCTL(sbb, all, EAS_SBB_ALL_SET)
SCHED_UNSET_BY_IOCTL(sbb, all, EAS_SBB_ALL_UNSET)
SCHED_SET_BY_IOCTL(sbb, group, EAS_SBB_GROUP_SET)
SCHED_UNSET_BY_IOCTL(sbb, group, EAS_SBB_GROUP_UNSET)
SCHED_SET_BY_IOCTL(sbb, task, EAS_SBB_TASK_SET)
SCHED_UNSET_BY_IOCTL(sbb, task, EAS_SBB_TASK_UNSET)
SCHED_SET_BY_IOCTL(sbb, active_ratio, EAS_SBB_ACTIVE_RATIO)
SCHED_SET_BY_IOCTL(thermal, headroom_interval_tick, EAS_GET_THERMAL_HEADROOM_INTERVAL_SET)
SCHED_SET_BY_IOCTL(task, idle_prefer, EAS_TASK_IDLE_PREFER_SET)
SCHED_SET_BY_IOCTL(sched, ctl_util_est, EAS_SCHED_CTL_EST)
SCHED_SET_BY_IOCTL(sched, ctl_target_freq_c0, EAS_SCHED_CTL_TARGET_FREQ_C0)
SCHED_SET_BY_IOCTL(sched, ctl_target_margin_c0, EAS_SCHED_CTL_TARGET_MARGIN_C0)
SCHED_SET_BY_IOCTL(sched, ctl_target_freq_c1, EAS_SCHED_CTL_TARGET_FREQ_C1)
SCHED_SET_BY_IOCTL(sched, ctl_target_margin_c1, EAS_SCHED_CTL_TARGET_MARGIN_C1)
SCHED_SET_BY_IOCTL(sched, ctl_target_freq_c2, EAS_SCHED_CTL_TARGET_FREQ_C2)
SCHED_SET_BY_IOCTL(sched, ctl_target_margin_c2, EAS_SCHED_CTL_TARGET_MARGIN_C2)
SCHED_SET_BY_IOCTL(sched, am_ctrl, EAS_SET_AM_CTRL)

SCHED_SET_MASK_BY_IOCTL(system, cpumask_int, PERF_RES_SCHED_SYSTEM_MASK, EAS_SET_SYSTEM_MASK)

SCHED_SET_MASK_BY_IOCTL(sched, ta_mask, PERF_RES_SCHED_TA_MASK, EAS_SET_CPUMASK_TA)
SCHED_SET_MASK_BY_IOCTL(sched, fg_mask, PERF_RES_SCHED_FG_MASK, EAS_SET_CPUMASK_FOREGROUND)
SCHED_SET_MASK_BY_IOCTL(sched, bg_mask, PERF_RES_SCHED_BG_MASK, EAS_SET_CPUMASK_BACKGROUND)
SCHED_SET_BY_IOCTL(sched, task_ls, EAS_SET_TASK_LS)
SCHED_UNSET_BY_IOCTL(sched, task_ls, EAS_UNSET_TASK_LS)

SCHED_SET_BY_IOCTL(sched, ignore_idle_util_ctrl, EAS_IGNORE_IDLE_UTIL_CTRL)

SCHED_SET_BY_IOCTL(sched, task_vip, EAS_SET_TASK_VIP)
SCHED_UNSET_BY_IOCTL(sched, task_vip, EAS_UNSET_TASK_VIP)
SCHED_SET_BY_IOCTL(sched, ta_vip, EAS_SET_TA_VIP)
SCHED_UNSET_BY_IOCTL(sched, ta_vip, EAS_UNSET_TA_VIP)
SCHED_SET_BY_IOCTL(sched, fg_vip, EAS_SET_FG_VIP)
SCHED_UNSET_BY_IOCTL(sched, fg_vip, EAS_UNSET_FG_VIP)
SCHED_SET_BY_IOCTL(sched, bg_vip, EAS_SET_BG_VIP)
SCHED_UNSET_BY_IOCTL(sched, bg_vip, EAS_UNSET_BG_VIP)
SCHED_SET_BY_IOCTL(sched, ls_vip, EAS_SET_LS_VIP)
SCHED_UNSET_BY_IOCTL(sched, ls_vip, EAS_UNSET_LS_VIP)

SCHED_SET_BY_IOCTL(sched, gear_migr_dn_pct, EAS_GEAR_MIGR_DN_PCT)
SCHED_SET_BY_IOCTL(sched, gear_migr_up_pct, EAS_GEAR_MIGR_UP_PCT)
SCHED_SET_BY_IOCTL(sched, gear_migr_cid, EAS_GEAR_MIGR_SET)
SCHED_UNSET_BY_IOCTL(sched, gear_migr_cid, EAS_GEAR_MIGR_UNSET)

SCHED_SET_BY_IOCTL(sched, task_gear_hints_start, EAS_TASK_GEAR_HINTS_START)
SCHED_SET_BY_IOCTL(sched, task_gear_hints_num, EAS_TASK_GEAR_HINTS_NUM)
SCHED_SET_BY_IOCTL(sched, task_gear_hints_reverse, EAS_TASK_GEAR_HINTS_REVERSE)
SCHED_SET_BY_IOCTL(sched, task_gear_hints_pid, EAS_TASK_GEAR_HINTS_SET)
SCHED_UNSET_BY_IOCTL(sched, task_gear_hints_pid, EAS_TASK_GEAR_HINTS_UNSET)
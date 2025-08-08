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
#include <errno.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include <ged/ged_log.h>
#include "common.h"
#include "utility_gpu.h"
#include "perfservice.h"

#define GPU_HARDLIMIT_ENABLED 1
#define PATH_GPU_POWER_POLICY     "/sys/class/misc/mali0/device/power_policy"
#define PATH_GPU_POWER_ONOFF_INTERVAL "/sys/class/misc/mali0/device/pm_poweroff"
#define PATH_CG_SPORTS "/sys/kernel/thermal/sports_mode"
#define PATH_CROSS_RANK "/sys/module/pgboost/parameters/kick_pgboost"
#define POWER_ONOFF_INTERVAL_DEFAULT 25

static int nSetFreqInit = 0;
static int nIsGpuFreqSupport = 0;
static int scn_gpu_freq_now = 0;
static int scn_gpu_freq_max_now = 0;
static int scn_hard_gpu_freq_now = 0;
static int scn_hard_gpu_freq_max_now = 0;
static int nGpuHighestFreqLevel = 0;
extern int nGpuFreqCount;

static void setGpuFreq(void);

int set_gpu_power_policy(int value, void *scn)
{
    ALOGI("set_gpu_power_policy: value:%d, scn:%p", value, scn);

    switch (value) {
        case 0:
            set_value(PATH_GPU_POWER_POLICY, "coarse_demand");
            break;
        case 1:
            set_value(PATH_GPU_POWER_POLICY, "always_on");
            break;
        default:
            break;
    }

    return 0;
}

int set_gpu_power_onoff_interval(int value, void *scn)
{
    char str[128];

    if(snprintf(str, sizeof(str), "400000 %d 0", value) < 0) {
        ALOGE("sprintf error");
        return 0;
    }
    LOG_I("str:%s, scn:%p", str, scn);
    set_value(PATH_GPU_POWER_ONOFF_INTERVAL, str);

    return 1;
}

int unset_gpu_power_onoff_interval(int value, void *scn)
{
    char str[128];

    if(snprintf(str, sizeof(str), "400000 %d 0", POWER_ONOFF_INTERVAL_DEFAULT) < 0) {
        ALOGE("sprintf error");
        return 0;
    }
    LOG_I("str:%s, scn:%p", str, scn);
    set_value(PATH_GPU_POWER_ONOFF_INTERVAL, str);

    return 1;
}

#define PROP_GPU_ACP_HINT   "vendor.powerhal.gpu.acp.hint"

int utility_gpu_acp_set([[maybe_unused]] int idx, [[maybe_unused]] void *scn)
{
    property_set(PROP_GPU_ACP_HINT, "1");
    return 1;
}

int utility_gpu_acp_unset([[maybe_unused]] int idx, [[maybe_unused]] void *scn)
{
    property_set(PROP_GPU_ACP_HINT, "");
    return 1;
}

int utility_gpu_acp_init([[maybe_unused]] int power_on_init)
{
    property_set(PROP_GPU_ACP_HINT, "");
    return 1;
}

int set_cg_policy(int value, void *scn)
{
    int ret = 0;

    ret = ged_set_mpts(value);
    LOG_D("ret=%d", ret);

    return 0;
}


static int checkGPUSupport(void)
{
    struct stat stat_buf;

    if(!nSetFreqInit) {
        nIsGpuFreqSupport = (0 == stat(PATH_GPUFREQ_COUNT, &stat_buf)) ? 1 : 0;
        nSetFreqInit = 1;
    }

    if(!nIsGpuFreqSupport)
        return 0;

    return 1;
}

int gpuInit(int level)
{
    if(checkGPUSupport()) {
        /* GPU info */
        get_gpu_freq_level_count(&nGpuFreqCount);
        if(nGpuFreqCount <= 0) {
            ALOGE("nGpuFreqCount error: %d", nGpuFreqCount);
            return -1;
        }
        /* Since Gpu Opp table range was from 0 to nGpuFreqCount-1, */
        /* we use nGpuFreqCount to represent free run.              */
        nGpuHighestFreqLevel = scn_gpu_freq_now = nGpuFreqCount - 1; // opp(n-1) is the lowest freq
        scn_hard_gpu_freq_now = nGpuFreqCount - 1;
        scn_gpu_freq_max_now = -1; // opp 0 is the highest freq
        scn_hard_gpu_freq_max_now = -1;
        ALOGI("nGpuFreqCount:%d", nGpuFreqCount);
    }

    return 0;
}

int setGPUFreqMinOPP(int level, void *scn)
{
    LOG_D("gpu min level: %d", level);
    if(checkGPUSupport()) {
        scn_gpu_freq_now = (level == -1) ? nGpuFreqCount - 1 : level;
        setGpuFreq();
    }else{
         LOG_E("gpu freq not support!!");
         return -1;
    }
    return 0;
}

int setGPUFreqHardMinOPP(int level, void *scn)
{
    LOG_D("gpu min level: %d", level);
    if(checkGPUSupport()) {
        scn_hard_gpu_freq_now = (level == -1) ? nGpuFreqCount - 1 : level;
        setGpuFreq();
    }else{
         LOG_E("gpu freq not support!!");
         return -1;
    }
    return 0;
}

int setGPUFreqMaxOPP(int level, void *scn) {
    LOG_D("gpu min level: %d", level);
    if(checkGPUSupport()) {
        scn_gpu_freq_max_now = (level == -1) ? 0 : level;
        setGpuFreq();
    }else{
         LOG_E("gpu freq not support!!");
         return -1;
    }
    return 0;
}

int setGPUFreqHardMaxOPP(int level, void *scn) {
    LOG_D("gpu min level: %d", level);
    if(checkGPUSupport()) {
        scn_hard_gpu_freq_max_now = (level == -1) ? 0 : level;
        setGpuFreq();
    }else{
         LOG_E("gpu freq not support!!");
         return -1;
    }
    return 0;
}

static void setGpuFreq(void)
{
    int levelToSet = 0;

    int freqToSet = scn_gpu_freq_now;
    int maxToSet = scn_gpu_freq_max_now;
    int final_min_freq = -1, final_max_freq = -1;

    // check whether freq max > freq min
    if (maxToSet > freqToSet)
        maxToSet = freqToSet;

#if GPU_HARDLIMIT_ENABLED
    int minHard = scn_hard_gpu_freq_now;
    int maxHard = scn_hard_gpu_freq_max_now;
    // check whether freq hard max > freq hard min
    if (maxHard > minHard)
        maxHard = minHard;
    ALOGI("[setGPUFreq] Soft min/max = (%d, %d); Hard min/max = (%d, %d)", freqToSet, maxToSet, minHard, maxHard);
    if (freqToSet > minHard) {
        freqToSet = minHard;
    }
    if (freqToSet < maxHard) {
        freqToSet = maxHard;
    }
    if (maxToSet < maxHard) {
        maxToSet = maxHard;
    }
    if (maxToSet > minHard) {
        maxToSet = minHard;
    }
#endif
    final_min_freq = (freqToSet < 0 || freqToSet >= nGpuFreqCount) ? (nGpuFreqCount - 1) : freqToSet;
    final_max_freq = (maxToSet < 0 || maxToSet >= nGpuFreqCount) ? 0 : maxToSet;
    ALOGI("[setGPUFreq] final min/max = (%d, %d)", final_min_freq, final_max_freq);
    set_gpu_freq_level(final_min_freq);
    set_gpu_freq_level_max(final_max_freq);
}

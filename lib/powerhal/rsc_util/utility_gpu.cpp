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

#define PATH_GPU_POWER_POLICY     "/sys/class/misc/mali0/device/power_policy"
#define PATH_GPU_POWER_ONOFF_INTERVAL "/sys/class/misc/mali0/device/pm_poweroff"
#define PATH_CG_SPORTS "/sys/kernel/thermal/sports_mode"
#define PATH_CROSS_RANK "/sys/module/pgboost/parameters/kick_pgboost"
#define POWER_ONOFF_INTERVAL_DEFAULT 25

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

    if(sprintf(str, "400000 %d 0", value) < 0) {
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

    if(sprintf(str, "400000 %d 0", POWER_ONOFF_INTERVAL_DEFAULT) < 0) {
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

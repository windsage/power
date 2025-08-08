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

#include "perfservice.h"
#include "common.h"
#include "utility_vpu.h"

#define VPU_CORE_MAX    2
#define VPU_BOOST_MAX   100
#define MDLA_BOOST_MAX  100

#define PATH_VPU_BOOST_CTRL   "/sys/kernel/debug/vpu/power"
#define PATH_MDLA_BOOST_CTRL  "/sys/kernel/debug/mdla/power"
#define PATH_APU_BOOST_CTRL  "/sys/kernel/debug/apusys/power"

static int vpu_min_now[VPU_CORE_MAX];
static int vpu_max_now[VPU_CORE_MAX];
static int mdla_min_now;
static int mdla_max_now;
static int mChipName;

void getChipName(int &chipName);

enum {
    USER_VPU0 = 0,
    USER_VPU1 = 1,
    USER_VPU2 = 2,
    USER_MDLA0 = 3,
    USER_MDLA1 = 4,
};

static void setVPUboost(int core, int boost_h, int boost_l)
{
    static int vpu_support = -1;
    struct stat stat_buf;
    char str[256], buf[256];



    if (vpu_support == -1) {
        if (mChipName == 6885)
            vpu_support = (0 == stat(PATH_APU_BOOST_CTRL, &stat_buf)) ? 1 : 0;
        else
            vpu_support = (0 == stat(PATH_VPU_BOOST_CTRL, &stat_buf)) ? 1 : 0;
    }

    LOG_D("vpu_support = %d mChipName:%d", vpu_support, mChipName);

    if (vpu_support != 1)
        return;

    boost_h = (boost_h == -1) ? VPU_BOOST_MAX : boost_h;
    boost_l = (boost_l == -1) ? 0 : boost_l;

    /* check ceiling must higher than floor*/
    if (boost_h < boost_l) {
        boost_h = boost_l;
        LOG_I("update vpu_boost: %d %d", boost_l, boost_h);
    }

    if (mChipName == 6885) {
        LOG_I("set vpu_boost: %d %d", boost_l, boost_h);
        str[0] = '\0';
        if(sprintf(buf, "power_hal %d %d %d ", USER_VPU0, boost_l, boost_h) < 0) {
            LOG_E("sprintf error");
            return;
        }
        strncat(str, buf, strlen(buf));
        str[strlen(str)-1] = '\0';
        LOG_V("set apu_boost: %s", str);
        set_value(PATH_APU_BOOST_CTRL, str);
    } else {
        LOG_I("set vpu_boost: %d %d %d", core, boost_l, boost_h);
        str[0] = '\0';
        if(sprintf(buf, "power_hal %d %d %d ", core+1, boost_l, boost_h) < 0) {
            LOG_E("sprintf error");
            return;
        }
        strncat(str, buf, strlen(buf));
        str[strlen(str)-1] = '\0';
        LOG_V("set vpu_boost: %s", str);
        set_value(PATH_VPU_BOOST_CTRL, str);
    }
}

static void setMDLAboost(int boost_h, int boost_l)
{
    static int mdla_support = -1;
    struct stat stat_buf;
    char str[256], buf[256];

    if (mdla_support == -1) {
        if (mChipName == 6885)
            mdla_support = (0 == stat(PATH_APU_BOOST_CTRL, &stat_buf)) ? 1 : 0;
        else
            mdla_support = (0 == stat(PATH_MDLA_BOOST_CTRL, &stat_buf)) ? 1 : 0;
    }

    LOG_D("mdla_support = %d mChipName:%d", mdla_support, mChipName);

    if (mdla_support != 1)
        return;

    LOG_I("set mdla_boost: %d  %d", boost_l, boost_h);

    boost_h = (boost_h == -1) ? MDLA_BOOST_MAX : boost_h;
    boost_l = (boost_l == -1) ? 0 : boost_l;

      /* check ceiling must higher than floor*/
    if (boost_h < boost_l) {
        boost_h = boost_l;
        LOG_I("update set mdla_boost: %d %d", boost_l, boost_h);
    }

    if (mChipName == 6885) {
        str[0] = '\0';
        if(sprintf(buf, "power_hal %d %d %d ", USER_MDLA0, boost_l, boost_h) < 0) {
            LOG_E("sprintf error");
            return;
        }
        strncat(str, buf, strlen(buf));
        str[strlen(str)-1] = '\0';
        LOG_V("set apu_boost: %s", str);
        set_value(PATH_APU_BOOST_CTRL, str);
    } else {
        str[0] = '\0';
        if(sprintf(buf, "power_hal %d %d ", boost_l, boost_h) < 0) {
            LOG_E("sprintf error");
            return;
        }
        strncat(str, buf, strlen(buf));
        str[strlen(str)-1] = '\0';
        LOG_V("set mdla_boost: %s", str);
        set_value(PATH_MDLA_BOOST_CTRL, str);
    }
}

int vpu_init(int poweron_init)
{
    int i;
    LOG_V("%d", poweron_init);

    getChipName(mChipName);

    for(i=0; i<VPU_CORE_MAX; i++) {
        vpu_min_now[i] = -1;
        vpu_max_now[i] = -1;
    }
    return 0;
}

int mdla_init(int poweron_init)
{
    LOG_V("%d", poweron_init);

    if (mChipName == 0)
        getChipName(mChipName);

    mdla_min_now = mdla_max_now = -1;
    return 0;
}

int setVpuFreqMin_core_0(int value, void *scn)
{
    LOG_V("%p", scn);
    vpu_min_now[0] = value;
    setVPUboost(0, vpu_max_now[0], vpu_min_now[0]);
    return 0;
}

int setVpuFreqMax_core_0(int value, void *scn)
{
    LOG_V("%p", scn);
    vpu_max_now[0] = value;
    setVPUboost(0, vpu_max_now[0], vpu_min_now[0]);
    return 0;
}

int setVpuFreqMin_core_1(int value, void *scn)
{
    LOG_V("%p", scn);
    vpu_min_now[1] = value;
    setVPUboost(1, vpu_max_now[1], vpu_min_now[1]);
    return 0;
}

int setVpuFreqMax_core_1(int value, void *scn)
{
    LOG_V("%p", scn);
    vpu_max_now[1] = value;
    setVPUboost(1, vpu_max_now[1], vpu_min_now[1]);
    return 0;
}

int setMdlaFreqMin(int value, void *scn)
{
    LOG_V("%p", scn);
    mdla_min_now = value;
    setMDLAboost(mdla_max_now, mdla_min_now);
    return 0;
}

int setMdlaFreqMax(int value, void *scn)
{
    LOG_V("%p", scn);
    mdla_max_now = value;
    setMDLAboost(mdla_max_now, mdla_min_now);
    return 0;
}

void getChipName(int &chipName)
{
    char szPlatformName[PROPERTY_VALUE_MAX];

    property_get("ro.board.platform", szPlatformName, "UNKNOWN");

    if (!strncmp(szPlatformName, "mt6885", 6)) {
        LOG_I("platformName: %s", szPlatformName);
        chipName = 6885;
    } else {
        LOG_I("platformName: %s", szPlatformName);
        chipName = 0xFFFFFFFF;
    }
}



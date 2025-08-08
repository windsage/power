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
#include <utils/Log.h>

#include "perfservice.h"
#include "common.h"

static int devfreqOppCount = 0;
static long long *devfreqTbl = NULL;


//using namespace std;

/* variable */
int initDvfsrcDevFreq(int power_on)
{
    LOG_V("%d", power_on);
    get_dvfsrc_devfreq_table(&devfreqOppCount, &devfreqTbl);
    return 0;
}

int setDvfsrcDevFreqOpp(int opp, void *scn)
{
    LOG_D("count:%d, opp:%d, scn:%p", devfreqOppCount, opp, scn);

    if (devfreqOppCount <= 0)
        return 0;

    if (opp >= 0 && opp < devfreqOppCount) {
        if (queryInteractiveState() == 1) {
            set_value(DVFSRC_DEVFREQ_SET_FREQ, devfreqTbl[opp]);
            LOG_I("opp:%d, freq:%lld", opp, devfreqTbl[opp]);
        }
    } else if (opp == -1)
        set_value(DVFSRC_DEVFREQ_SET_FREQ, 0);

    return 0;
}

int setThermalDvfsrcDevFreqOpp(int enable, void *scn)
{
    LOG_D("enable:%d, scn:%p", enable, scn);

    if (devfreqOppCount <= 0)
        return 0;

    if (enable) {
        set_value(DVFSRC_DEVFREQ_SET_MAX_FREQ, devfreqTbl[devfreqOppCount-1]);
        LOG_I("opp:%d, freq:%lld", devfreqOppCount-1, devfreqTbl[devfreqOppCount-1]);
    } else
        set_value(DVFSRC_DEVFREQ_SET_MAX_FREQ, devfreqTbl[0]);

    return 0;
}

int set_hugepage_policy(int value, void *scn)
{
    FILE *file;
    int ret, enable = value;

    file = fopen("/sys/kernel/mm/transparent_hugepage/defrag", "w");
    if (file == NULL) return -1;
    if (enable)
        ret = fputs("always", file);
    else
        ret = fputs("madvise", file);
    if (ret < 0) goto err_close;
    ret = fclose(file);
    if (ret != 0) goto err_exit;

    file = fopen("/sys/kernel/mm/transparent_hugepage/khugepaged/defrag", "w");
    if (file == NULL) return -2;
    if (enable)
        ret = fputs("0", file);
    else
        ret = fputs("1", file);
    if (ret < 0) goto err_close;
    ret = fclose(file);
    if (ret != 0) goto err_exit;

    file = fopen("/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs", "w");
    if (file == NULL) return -3;
    if (enable)
        ret = fputs("1000000000", file);
    else
        ret = fputs("10000", file);
    if (ret < 0) goto err_close;
    ret = fclose(file);
    if (ret != 0) goto err_exit;

    file = fopen("/sys/kernel/mm/transparent_hugepage/hugepages-32kB/enabled", "w");
        if (file == NULL) return -4;
        if (enable)
                ret = fputs("always", file);
        else
                ret = fputs("never", file);
        if (ret < 0) goto err_close;
        ret = fclose(file);
        if (ret != 0) goto err_exit;

    file = fopen("/sys/kernel/mm/transparent_hugepage/hugepages-16kB/enabled", "w");
        if (file == NULL) return -5;
        if (enable)
                ret = fputs("always", file);
        else
                ret = fputs("never", file);
        if (ret < 0) goto err_close;
        ret = fclose(file);
        if (ret != 0) goto err_exit;

    file = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "w");
        if (file == NULL) return -6;
        if (enable)
                ret = fputs("always", file);
        else
                ret = fputs("never", file);
        if (ret < 0) goto err_close;
        ret = fclose(file);
        if (ret != 0) goto err_exit;

    return 0;

err_close:
    if (errno != 0) ALOGE("ErrorNo: %d", errno);
    ret = fclose(file);
    if (ret != 0) goto err_exit;
    return -7;
err_exit:
    if (errno != 0) ALOGE("ErrorNo: %d", errno);
    return -8;
}

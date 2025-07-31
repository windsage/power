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

#include <dlfcn.h>
#include <stdio.h>
#include <log/log.h>
#include <utils/Log.h>
#include <cutils/uevent.h>
#include <cutils/properties.h>
#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <pthread.h>
#include "mtkperf_resource.h"
#include "power_pelt_handler.h"
#include "utility_sched.h"
#include "utility_sf.h"
#include "common.h"

static int devfd = -1;
static int pelt_hdl = 0;
static int fpsgo_is_boosting = 0;
static int cpufreq_is_boosting = 0;
static int perf_api_supported = 0;

typedef int (*perf_lock_acq)(int, int, int[], int);
typedef int (*perf_lock_rel)(int);

static int  (*perfLockAcq)(int, int, int[], int) = NULL;
static int  (*perfLockRel)(int) = NULL;

static int load_perf_api(void) {
    void *func = NULL, *lib_handle = NULL;
    perf_api_supported = 0;

    lib_handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);

    if (lib_handle == NULL) {
        ALOGD("power hal: dlopen fail: %s\n", dlerror());
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_acq");
    perfLockAcq = reinterpret_cast<perf_lock_acq>(func);

    if (perfLockAcq == NULL) {
        ALOGD("power hal: perfLockAcq error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    func = dlsym(lib_handle, "perf_lock_rel");
    perfLockRel = reinterpret_cast<perf_lock_rel>(func);

    if (perfLockRel == NULL) {
        ALOGD("power hal: perfLockRel error: %s\n", dlerror());
        dlclose(lib_handle);
        return -1;
    }

    ALOGI("load powerhal api successfully \n");
    perf_api_supported = 1;
    return 0;
}

int check_ioctl_valid() {
    if (devfd >= 0) {
        return 0;
    } else if (devfd == -1) {
        devfd = open(PATH_IOCTL, O_RDONLY);
        if (devfd < 0 && errno == ENOENT) {
            devfd = -2;
        }
        if (devfd == -1) {
            LOG_E("Cannot open %s (%s)", PATH_IOCTL, strerror(errno));
            return -1;
        }
    } else if (devfd == -2) {
        return -2;
    }
    return 0;
}

void pelt_boost_cpu() {
    int pelt_halflife_value = 8;
    int perf_lock_rsc[] = {
        PERF_RES_SCHED_PELT_HALFLIFE, pelt_halflife_value
        };
    int perf_lock_rsc_size = sizeof(perf_lock_rsc)/sizeof(int);

    if (get_sf_low_power_hint_status() == 0)
        return;

    LOG_I("fpsgo_is_boosting:%d, cpufreq_is_boosting:%d",
        fpsgo_is_boosting, cpufreq_is_boosting);

    if (!perfLockAcq || !perfLockRel)
        load_perf_api();

    if (!perf_api_supported)
        return;

    if (fpsgo_is_boosting || cpufreq_is_boosting) {
        pelt_hdl = perfLockAcq(pelt_hdl, 0, perf_lock_rsc, perf_lock_rsc_size);
    } else {
        perfLockRel(pelt_hdl);
        pelt_hdl = 0;
    }
}

struct _CPU_CTRL_PACKAGE get_boost_cmd(void) {
    int ioctl_ret = 0;
    int ioctl_valid = check_ioctl_valid();
    _CPU_CTRL_PACKAGE msg;
    msg.cmd = -1;
    msg.value = -1;

    if (ioctl_valid == 0) {
        ioctl_ret = ioctl(devfd, NOTIFY_BOOST, &msg);
    } else {
        LOG_E("ioctl_valid: %d", ioctl_valid);
        sleep(60);
    }

    if (ioctl_ret != 0) {
        LOG_E("ioctl_ret: %d", ioctl_ret);
        sleep(60);
    }

    return msg;
}

static void* pelt_handler(void *data)
{
    struct _CPU_CTRL_PACKAGE msg;

    LOG_I("start pelt handler");

    while (1) {
        msg = get_boost_cmd();
        switch (msg.cmd) {
            case UNKNOWN:
                LOG_E("UNKNOWN cmd:%d", msg.cmd);
                sleep(10);
                break;
            case FPSGO_BOOST:
                LOG_I("[FPSGO_BOOST] %d", msg.value);
                fpsgo_is_boosting = msg.value;
                pelt_boost_cpu();
                break;
            case CPUFREQ_BOOST:
                LOG_I("[CPUFREQ_BOOST] %d", msg.value);
                cpufreq_is_boosting = msg.value;
                pelt_boost_cpu();
                break;
            default:
                LOG_E("default cmd:%d", msg.cmd);
                break;
        }
    }

    return NULL;
}

int create_pelt_handler(int power_on)
{
    pthread_t handlerThread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&handlerThread, &attr, pelt_handler, NULL);
    pthread_setname_np(handlerThread, "mtk_pelt_handler");

    return 0;
}

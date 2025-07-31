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
#include <cutils/properties.h>
#include <errno.h>
#include <sys/stat.h>

#include "perfservice.h"
#include "common.h"
#include "utility_sys.h"

#define MAX_CPU_NUM 16
#define PATH_CPU_DMA_LATENCY "/dev/cpu_dma_latency"
#define PATH_POWERHAL_IOCTL "/proc/perfmgr_powerhal/ioctl_powerhal"

//using namespace std;

/* variable */
static int sys_sched_cpu_prefer = 0;
static int eas_ctrl_cpu_prefer = 0;
static int cpu_ctrl_isolation_enabled = 0;
static int cpu_num = get_cpu_num();
static int isolation_cpu_count[MAX_CPU_NUM] = {0};
static int isolation_cpu_max = (1 << cpu_num) - 1;
static int fd_cpu_dma_latency = 0;
int powerhalsysdevfd = -1;

/* function */
int initTaskPreferCpu(int power_on)
{
    struct stat stat_buf;

    LOG_V("%d", power_on);
    sys_sched_cpu_prefer = (0 == stat(PATH_TASK_PREFER_CPU, &stat_buf)) ? 1 : 0;
    eas_ctrl_cpu_prefer = (0 == stat(PATH_EAS_CTRL_TASK_PREFER_CPU, &stat_buf)) ? 1 : 0;
    return 0;
}

int setTaskPreferCpu_big(int tid, void *scn)
{
    char str[64];
    if(!sys_sched_cpu_prefer && !eas_ctrl_cpu_prefer)
        return -1;

    LOG_D("%d, scn:%p", tid, scn);
    if(snprintf(str, sizeof(str)-1, "%d 1", tid) < 0) {
        LOG_E("snprintf error");
        return 0;
    }

    if (sys_sched_cpu_prefer)
        set_value(PATH_TASK_PREFER_CPU, str);
    else if (eas_ctrl_cpu_prefer)
        set_value(PATH_EAS_CTRL_TASK_PREFER_CPU, str);
    return 0;
}

int unsetTaskPreferCpu_big(int tid, void *scn)
{
    char str[64];
    if(!sys_sched_cpu_prefer && !eas_ctrl_cpu_prefer)
        return -1;

    LOG_D("%d, scn:%p", tid, scn);
    if(snprintf(str, sizeof(str)-1, "%d 0", tid) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    if (sys_sched_cpu_prefer)
        set_value(PATH_TASK_PREFER_CPU, str);
    else if (eas_ctrl_cpu_prefer)
        set_value(PATH_EAS_CTRL_TASK_PREFER_CPU, str);
    return 0;
}

int setTaskPreferCpu_little(int tid, void *scn)
{
    char str[64];
    if(!sys_sched_cpu_prefer && !eas_ctrl_cpu_prefer)
        return -1;

    LOG_D("%d, scn:%p", tid, scn);
    if(snprintf(str, sizeof(str)-1, "%d 2", tid) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    if (sys_sched_cpu_prefer)
        set_value(PATH_TASK_PREFER_CPU, str);
    else if (eas_ctrl_cpu_prefer)
        set_value(PATH_EAS_CTRL_TASK_PREFER_CPU, str);
    return 0;
}

int unsetTaskPreferCpu_little(int tid, void *scn)
{
    char str[64];
    if(!sys_sched_cpu_prefer && !eas_ctrl_cpu_prefer)
        return -1;

    LOG_D("%d, scn:%p", tid, scn);
    if(snprintf(str, sizeof(str)-1, "%d 0", tid) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    if (sys_sched_cpu_prefer)
        set_value(PATH_TASK_PREFER_CPU, str);
    else if (eas_ctrl_cpu_prefer)
        set_value(PATH_EAS_CTRL_TASK_PREFER_CPU, str);
    return 0;
}

int init_isolation_cpu(int power_on)
{
    struct stat stat_buf;

    LOG_V("%d", power_on);
    cpu_ctrl_isolation_enabled = (0 == stat(PATH_CPU_CTRL_ISO_CPU, &stat_buf)) ? 1 : 0;

    return 0;
}

int set_isolation_cpu(int iso_cpu_enable, void *scn)
{
    char str[64];
    int i, iso_cpu_mask;

    if (!cpu_ctrl_isolation_enabled)
        return -1;

    LOG_I("iso_cpu_enable:%d 0x%x, scn:%p", iso_cpu_enable, iso_cpu_enable, scn);
    if (iso_cpu_enable < 0 || iso_cpu_enable >= isolation_cpu_max) {
        LOG_E("iso_cpu_enable:%d 0x%x. Cannot isolate all CPUs !", iso_cpu_enable, iso_cpu_enable);
        return -1;
    }

    isolation_cpu_count[i] = {0};
    for (i = 0; i < cpu_num; i++) {
        if ((iso_cpu_enable & (1 << i)) > 0)
            isolation_cpu_count[i]++;
    }

    iso_cpu_mask = 0;
    for (i = 0; i < cpu_num ; i++) {
        if (isolation_cpu_count[i] > 0)
            iso_cpu_mask += (1 << i);
    }
    LOG_I("isolation_cpu_mask: 0x%x", iso_cpu_mask);

    if(snprintf(str, sizeof(str)-1, "%d", iso_cpu_mask) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    set_value(PATH_CPU_CTRL_ISO_CPU, str);

    return 0;
}

int unset_isolation_cpu(int iso_cpu_enable, void *scn)
{
    char str[64];
    int i, iso_cpu_mask;

    if (!cpu_ctrl_isolation_enabled)
        return -1;

    LOG_I("iso_cpu_enable:%d 0x%x, scn:%p", iso_cpu_enable, iso_cpu_enable, scn);
    if (iso_cpu_enable < 0 || iso_cpu_enable >= isolation_cpu_max) {
        LOG_E("iso_cpu_enable:%d 0x%x. Cannot isolate all CPUs !", iso_cpu_enable, iso_cpu_enable);
        return -1;
    }

    for (i = 0; i < cpu_num; i++) {
        isolation_cpu_count[i] = 0;
    }

    iso_cpu_mask = 0;
    for (i = 0; i < cpu_num ; i++) {
        if (isolation_cpu_count[i] > 0)
            iso_cpu_mask += (1 << i);
    }
    LOG_I("isolation_cpu_mask: 0x%x", iso_cpu_mask);

    if(snprintf(str, sizeof(str)-1, "%d", iso_cpu_mask) < 0) {
        LOG_E("snprintf error");
        return 0;
    }
    set_value(PATH_CPU_CTRL_ISO_CPU, str);

    return 0;
}
int init_cpuidle(int power_on)
{
    int32_t v = 0;

	fd_cpu_dma_latency = open(PATH_CPU_DMA_LATENCY, O_WRONLY);
    LOG_D("fd:%d", fd_cpu_dma_latency);

    if (fd_cpu_dma_latency < 0) {
        char *err_str_open = strerror(errno);
        LOG_E("open fail: %d, %s, fd:%d", errno, err_str_open, fd_cpu_dma_latency);
		return -1;
	}

	v = get_cpu_dma_latency_value();
	if (write(fd_cpu_dma_latency, &v, sizeof(v)) != sizeof(v)) {
        char *err_str_write = strerror(errno);
        LOG_E("write fail: %d, %s, fd:%d", errno, err_str_write, fd_cpu_dma_latency);
		return -1;
	}

    if (fd_cpu_dma_latency && close(fd_cpu_dma_latency)) {
        char *err_str_close = strerror(errno);
        LOG_E("close fail: %d, %s, fd:%d", errno, err_str_close, fd_cpu_dma_latency);
		return -1;
	}

    return 0;
}

int set_cpuidle(int value, void *scn)
{
	int32_t v = 0;

    LOG_I("enable:%d, fd:%d, scn:%p", value, fd_cpu_dma_latency, scn);

    if (value == 1) {
        if (fd_cpu_dma_latency && close(fd_cpu_dma_latency)) {
            char *err_str_close = strerror(errno);
            LOG_E("close fail: %d, %s, fd:%d", errno, err_str_close, fd_cpu_dma_latency);
            return -1;
	    }
    } else {
        fd_cpu_dma_latency = open(PATH_CPU_DMA_LATENCY, O_WRONLY);
        if (fd_cpu_dma_latency < 0) {
            char *err_str_open = strerror(errno);
            LOG_E("open fail: %d, %s, fd:%d", errno, err_str_open, fd_cpu_dma_latency);
            return -1;
        }

        v = get_cpu_dma_latency_value();
        if (write(fd_cpu_dma_latency, &v, sizeof(v)) != sizeof(v)) {
            char *err_str_write = strerror(errno);
            LOG_E("write fail: %d, %s, fd:%d", errno, err_str_write, fd_cpu_dma_latency);
            return -1;
        }
    }

	return 0;
}

int set_webp_use_threads_mode(int value, void *scn) {
    int ret = 0;
    char property_value[PROPERTY_VALUE_MAX];

    if (sprintf(property_value, "%d", value) < 0) {
        ALOGE("[set_webp_use_threads_mode] sprintf failed!");
        return -1;
    }

    ALOGI("set[vendor.mtk.webp.use.threads]: %d %p", value, scn);

    ret = property_set("vendor.mtk.webp.use.threads", property_value);

    return ret;
}

static inline int check_powerhal_ioctl_valid(void)
{
    if (powerhalsysdevfd >= 0) {
        return 0;
    } else if (powerhalsysdevfd == -1) {
        powerhalsysdevfd = open(PATH_POWERHAL_IOCTL, O_RDONLY);
        // file not exits
        if (powerhalsysdevfd < 0 && errno == ENOENT) {
            powerhalsysdevfd = -2;
            ALOGE("File not exits");
        }

        // file exist, but can't open
        if (powerhalsysdevfd == -1) {
            ALOGE("Can't open %s: %s", PATH_POWERHAL_IOCTL, strerror(errno));
            return -1;
        }
        // file not exist
    } else if (powerhalsysdevfd == -2) {
        ALOGE("File not exits second");
        return -2;
    }
    return 0;
}

int dsu_cci_sport_mode(int value, void *scn)
{
    ALOGI("value: %d", value);
    if (check_powerhal_ioctl_valid() == 0) {
        LOG_D("sys ioctl check_powerhal_ioctl_valid");
        _POWERHAL_PACKAGE data;
        data.value = value;
        ioctl(powerhalsysdevfd, DSU_CCI_SPORT_MODE, &data);
        return 0;
    }
    LOG_E("sys ioctl check_powerhal_ioctl_valid ERROR");

    return -2;
}

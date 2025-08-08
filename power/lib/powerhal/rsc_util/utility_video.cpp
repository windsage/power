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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "common.h"
#include "utility_video.h"
#include "perfservice.h"

#undef LOG_TAG
#define LOG_TAG "VideoUtility"

int set_video_color_convert_mode(int value, void *scn)
{
    int ret = 0;
    char property_value[PROPERTY_VALUE_MAX];

    if (snprintf(property_value, sizeof(property_value), "%d", value) < 0) {
        ALOGE("[set_video_color_convert_mode] sprintf failed!");
        return -1;
    }

    ALOGI("set_video_color_convert_mode: %d %p", value, scn);

    ret = property_set("vendor.mtkomx.color.convert", property_value);
    ret = property_set("vendor.mtk.c2.vdec.fmt.disabled.whitelist", property_value);

    return ret;
}

int set_video_high_cpu_freq_mode(int value, void *scn)
{
    int ret = 0, pid = 0;
    char property_value[PROPERTY_VALUE_MAX];

    getForegroundInfo(NULL, &pid, NULL);

    if(pid == 0) {
        return -1;
    }

    if (snprintf(property_value, sizeof(property_value), "%d_%d", pid, value) < 0) {
        ALOGE("[set_video_high_cpu_freq_mode] sprintf failed!");
        return -1;
    }

    ALOGI("set_video_high_cpu_freq_mode: %d %p", value, scn);

    ret = property_set("vendor.mtk.c2.scenario.transcoding", property_value);

    return ret;
}

int set_video_low_latency_mode(int value, void *scn)
{
    int ret = 0, pid = 0;
    char property_value[PROPERTY_VALUE_MAX];

    getForegroundInfo(NULL, &pid, NULL);

    if(pid == 0) {
        return -1;
    }

    if (snprintf(property_value, sizeof(property_value), "%d_%d", pid, value) < 0) {
        ALOGE("[set_video_low_latency_mode] sprintf failed!");
        return -1;
    }

    ALOGI("set_video_low_latency_mode: %d %p", value, scn);

    ret = property_set("vendor.mtk.c2.scenario.lowlatency", property_value);

    return ret;
}
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

#define LOG_TAG "mtkpower_client"
#define MAGTSYNC_LIB_FULL_NAME "libmagtsync.so"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <log/log.h>
#include <inttypes.h>
#include <vector>
#include <cutils/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <aidl/vendor/mediatek/hardware/mtkpower_applist/IMtkpower_applist.h>
#include "utility_magt.h"

typedef int (*magt_notify_foreground_app)(const char *, const char *, int, int, int);
int (*magtNotifyFGApp)(const char *, const char *, int, int, int) = NULL;

int magt_prop_supported = -1, magt_supported = -1;

int load_magt_api(void)
{
    void *handle = NULL, *func = NULL;
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int prop_value = -1;

    if (magt_prop_supported == -1) {
        property_get(PROP_MAGT_SUPPORT, prop_content, "0");
        magt_prop_supported = strtol(prop_content, NULL, 10);
    }

    if (magt_prop_supported == 0) {
        ALOGE("magt not supported, skip magt notification");
        return -1;
    }

    handle = dlopen(MAGTSYNC_LIB_FULL_NAME, RTLD_NOW);
    if (handle == NULL) {
        ALOGE("dlopen error: %s, skip magt sync", dlerror());
        magt_supported = 0;
        return -1;
    } else {
        func = dlsym(handle, "notifyAppState");
        magtNotifyFGApp = reinterpret_cast<magt_notify_foreground_app>(func);

        if (magtNotifyFGApp == NULL) {
            ALOGE("MAGT notifyAppState error: %s", dlerror());
            magt_supported = 0;
            dlclose(handle);
            return -1;
        }
        magt_supported = 1;
    }

    ALOGD("load magt successfully");

    return 0;
}

int magtNotifyForegroundApp(const char *packname, const char *actname, uint32_t pid, uint32_t activityId,
                            int32_t status, uint32_t uid) {
    if (magt_prop_supported == 0 || magt_supported == 0)
        return -1;

    if (magtNotifyFGApp == NULL && load_magt_api() == -1)
        return -1;

    ALOGD("[magtNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

    if (magtNotifyFGApp)
        magtNotifyFGApp(packname, actname, pid, status, uid);

    return  0;
}

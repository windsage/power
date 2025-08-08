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
#define FBC_LIB_FULL_NAME  "libgamescn.so"

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
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <aidl/vendor/mediatek/hardware/mtkpower_applist/IMtkpower_applist.h>
#include "utility_gcn.h"

typedef int (*fbc_notify_foreground_app)(const char *, const char *, int, int, int);
int (*powerNotifyFGApp)(const char *, const char *, int, int, int) = NULL;

static int fbc_api_inited = 0;

int load_fbc_api(void)
{
    void *handle = NULL, *func = NULL;

    handle = dlopen(FBC_LIB_FULL_NAME, RTLD_NOW);
    if (handle == NULL) {
        if (!fbc_api_inited){
            ALOGD("dlopen error: %s", dlerror());
            fbc_api_inited = 1;
        }
        return -1;
    }

    func = dlsym(handle, "powerNotifyForegroundApp");
    powerNotifyFGApp = reinterpret_cast<fbc_notify_foreground_app>(func);
    if (powerNotifyFGApp == NULL) {
         ALOGD("powerNotifyFGApp error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    ALOGD("load fbc successfully");

    return 0;
}

int powerNotifyForegroundApp(const char *packname, const char *actname, uint32_t pid, uint32_t activityId,
                            int32_t status, uint32_t uid) {
    if (powerNotifyFGApp == NULL)
        load_fbc_api();

    ALOGD("[powerNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

    if (powerNotifyFGApp)
        powerNotifyFGApp(packname, actname, status, pid, uid);

    return  0;
}

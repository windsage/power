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
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "common.h"
#include "utility_touch.h"

#define PATH_TOUCH_CHANGE_RATE    "/sys/devices/platform/mtk-tpd2.0/change_rate"
#undef LOG_TAG
#define LOG_TAG "TouchUtility"
#define TOUCH_PATH "libtouch_ll_wrap.so"
typedef int (*power_notify_AppState)(const char*, uint32_t);
static int (*notifyAppState)(const char*, uint32_t) = NULL;
static bool initlib = false;

static void initTouchLibrary() {
    void* sDlOpHandler = NULL;
    void* func = NULL;
    if (initlib) return;
    initlib = true;
    sDlOpHandler = dlopen(TOUCH_PATH, RTLD_NOW);
    if (sDlOpHandler == NULL) {
        ALOGE("[%s] dlopen failed in %s: %s",
                __FUNCTION__, TOUCH_PATH, dlerror());
        return;
    }
    func = dlsym(sDlOpHandler, "Touch_notifyAppState");
    notifyAppState = (power_notify_AppState)(func);
    const char* dlsym_error = dlerror();

    if (notifyAppState == NULL) {
        ALOGE("notifyAppState error: %s", dlsym_error);
        dlclose(sDlOpHandler);
        return;
    }

    // reset errors
    dlerror();

    ALOGD("[%s] completed", __FUNCTION__);
}

void notifyTouchForegroundApp(const char *packname, int32_t uid) {
    initTouchLibrary();
    if (notifyAppState == NULL) {
        ALOGI("notifyAppState error = NULL");
        return;
    }
    notifyAppState(packname, uid);
    ALOGI("notifyTouchForegroundApp pack:%s, uid:%d", packname, uid);
}

int setFreqBoost(int value, void *scn)
{
    int ret = 0;

    ALOGV("setFreqBoost: %p", scn);

    ret = set_value(PATH_TOUCH_CHANGE_RATE, value);
    if (ret == 0)
        ALOGI("setFreqBoost err,please check chang_rate node");
    else
        ALOGI("setFreqBoost success: value:%d", value);
    return ret;
}

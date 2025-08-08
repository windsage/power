
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
#define APPLIST_PROP_ENABLE "persist.system.powerhal.applist_enable"

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
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPower.h>
#include <aidl/vendor/mediatek/hardware/mtkpower_applist/IMtkpower_applist.h>
#include "utility_applist.h"

using android::hardware::Return;
using ::ndk::SpAIBinder;


/* sync enums from /power_applist/service/aidl/default/mtkpower_applist.h  */
enum SysInfo {
    SYSINFO_CUR_MODE = 0,
    SYSINFO_RELOAD_APPLIST_CONFIG,
    SYSINFO_TEST_MODE,
    SYSINFO_APP_LIST_CONFIG,
    CONTROLLER_TYPE_APP_CFG,
    CONTROLLER_TYPE_THREAD_HINT,
};


static bool gMtkPowerAPPListExists = true;
static bool gMtkPowerHalExists = true;
static int applistServiceVersion = -1;
static int applist_support = -1;
using namespace vendor::mediatek::hardware::mtkpower::V1_0;
using aidl::vendor::mediatek::hardware::mtkpower_applist::IMtkpower_applist;
std::shared_ptr<IMtkpower_applist> gMtkPowerAPPList;
static android::sp<IMtkPower> gMtkPowerHal = nullptr;


static bool getMtkPowerAPPList();

int check_applist_supported()
{
    int prop_value = 0;
    char prop_content[128] = "\0";
    property_get(APPLIST_PROP_ENABLE, prop_content, "0");
    prop_value = atoi(prop_content);

    if (applist_support == 1) {
        return 1;
    } else if (applist_support == -1) {
        applist_support = prop_value;
        return applist_support;
    } else if (applist_support == 0) {
        return 0;
    }

    return 0;
}

static void processReturn(const Return<void> &ret, const char* functionName) {
    if(!ret.isOk()) {
        ALOGE("%s() failed: Power HAL service not available", functionName);
        gMtkPowerHal = nullptr;
    }
}

static bool getMtkPowerHal() {
    if (gMtkPowerHal == nullptr) {
        gMtkPowerHal = IMtkPower::getService();
        if (gMtkPowerHal != nullptr) {
            ALOGI("Loaded mtkpower HAL service");
        } else {
            ALOGE("Couldn't load power HAL service");
            gMtkPowerHalExists = false;
        }
    }
    return (gMtkPowerHal != nullptr);
}

static bool getMtkPowerAPPList() {
    if (gMtkPowerAPPList == nullptr && IMtkpower_applist::descriptor != nullptr) {
        static const std::string kInstance = std::string() + IMtkpower_applist::descriptor + "/default";
        gMtkPowerAPPList = IMtkpower_applist::fromBinder(SpAIBinder(AServiceManager_getService(kInstance.c_str())));

        if (gMtkPowerAPPList != nullptr) {
            ALOGI("Loaded mtkpower APPList service");
            gMtkPowerAPPList->getInterfaceVersion(&applistServiceVersion);
            ALOGI("applistServiceVersion=%d", applistServiceVersion);
        } else {
            ALOGE("Couldn't load mtkpower applist service");
            gMtkPowerAPPListExists = false;
        }
    }

    return (gMtkPowerAPPList != nullptr);
}

int applistNotifyForegroundApp(const char *packname, const char *actname, uint32_t pid, uint32_t activityId,
                                int32_t status, uint32_t uid) {
    if (check_applist_supported() == 1) {
        if (getMtkPowerAPPList() && gMtkPowerAPPList != nullptr) {
            int32_t wrap_ver = IMtkpower_applist::version;
            int32_t applist_ver = 0;
            ndk::ScopedAStatus ret = gMtkPowerAPPList->notifyAppStateInfo(packname, actname, pid, activityId, status, uid);
            if(gMtkPowerAPPList->getInterfaceVersion(&applist_ver).isOk() && applist_ver < wrap_ver) {  // old aidl version
                ndk::ScopedAStatus ret = gMtkPowerAPPList->notifyAppState(packname, actname, pid, status, uid);
            }

            ALOGD("[applistNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s",__FUNCTION__);
            Return<void> ret = gMtkPowerHal->notifyAppState(packname, actname, pid, status, uid);
            ALOGI("[Legacy][PowerHal_Wrap_notifyAppState] %s/%s pid=%d state:%d", packname, actname, pid, activityId, status);
            processReturn(ret, __FUNCTION__);
        }
    }


    return  0;
}

void applistSetThreadHintControllerEnabled(bool enable)
{
    ALOGI("enable=%d", enable);

    if (check_applist_supported() == 1) {
        if (getMtkPowerAPPList() && gMtkPowerAPPList != nullptr) {
            if (applistServiceVersion >= 3) {
                char buf[10];
                buf[0] = '\0';
                int s = snprintf(buf, sizeof(buf), "%d", enable);
                if (s < 0) {
                    ALOGE("snprintf error");
                    return;
                }

                auto r = gMtkPowerAPPList->setSysInfo(CONTROLLER_TYPE_THREAD_HINT, buf);

            } else {
                ALOGE("[%s] Not support! Device not launching with mtkpower_applist-V3 and beyond. (applistServiceVersion=%d)",
                    __FUNCTION__, applistServiceVersion);
            }

        }
    }
}

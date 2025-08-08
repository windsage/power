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
#define MFRC_VER (2)

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
#include <aidl/vendor/mediatek/hardware/gpuserv/IGpuService.h>
#include <cutils/properties.h>
#include <common.h>
#include "utility_mfrc.h"
#include "mfrc/config.h"

using ::ndk::SpAIBinder;

static int IGpuService_supported = -1;
using aidl::vendor::mediatek::hardware::gpuserv::IGpuService;
std::shared_ptr<IGpuService> gGpuService;

char const* kDefaultAppConfigPaths[] = {
    "/system/etc/mfrc.cfg",
    NULL
};

using namespace MTK::MFRC;
std::shared_ptr<WL> gMfrcAppConfig = nullptr;

static int check_IGpuService_supported()
{
    const char * descriptor = aidl::vendor::mediatek::hardware::gpuserv::IGpuService::descriptor;

    if(IGpuService_supported != -1)
        return IGpuService_supported;

    if (descriptor) {
        static const std::string IGpuService_instance = std::string() + descriptor + "/default";
        IGpuService_supported = AServiceManager_isDeclared(IGpuService_instance.c_str());
        ALOGI("IGPUService_isDeclared %d", IGpuService_supported);
        return IGpuService_supported;
    } else {
        ALOGE("descriptor is null");
        return 0;
    }
}

static void getGpuService() {
    if (IGpuService::descriptor == nullptr) {
        ALOGD("GPU service is not supported.");
        return;
    }

    if (gGpuService != nullptr) {
        return;
    }

    static const std::string kInstance = std::string() + IGpuService::descriptor + "/default";
    AIBinder* binder = AServiceManager_checkService(kInstance.c_str());

    if (binder == nullptr) { // GPU service may not be running.
        ALOGD("Failed to check GPU service");
        return;
    }

    gGpuService = IGpuService::fromBinder(SpAIBinder(binder));

    if (gGpuService != nullptr) {
        ALOGI("Loaded GPU service");
    } else {
        ALOGE("Couldn't load GPU service");
    }
}

int mfrcNotifyForegroundApp(const char *packname, const char *actname, uint32_t pid, uint32_t activityId,
                            int32_t status, uint32_t uid) {
    if (!property_get_bool("ro.vendor.mtk.gpu.service.lazy", false)) { // w/o Lazy loading.
        int32_t aidl_ret = 0, mfrc_ver = 0, wrap_ver = IGpuService::version;

        getGpuService();

        if (check_IGpuService_supported() && gGpuService != nullptr && gGpuService->getInterfaceVersion(&mfrc_ver).isOk() && mfrc_ver >= wrap_ver) {
            ALOGD("[mfrcNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

            ndk::ScopedAStatus ret = gGpuService->NotifyAppState(packname, actname, pid, activityId, status, uid, &aidl_ret);
        }

        return  0;
    }
    else { // w/ Lazy loading.
        // Checking the App list.
        if (!gMfrcAppConfig) {
            char propertyBuffer[PROPERTY_VALUE_MAX];

            if (property_get("vendor.mtk.mfrc.applist.path", propertyBuffer, "") > 0) {
                ALOGI("Read MFRC app list: %s", propertyBuffer);
                char* appConfigPaths[] = {
                    propertyBuffer,
                    NULL
                };
                gMfrcAppConfig = std::make_shared<WL>(appConfigPaths);
            } else {
                ALOGI("Read MFRC app list: %s", kDefaultAppConfigPaths[0]);
                gMfrcAppConfig = std::make_shared<WL>(kDefaultAppConfigPaths);
            }
        }

        bool isMFRCEnabled = strcmp(gMfrcAppConfig->get_setting(packname,"mfrc"), "enable") == 0;

        if (!property_get_bool("vendor.debug.forcibly_enable_mfrc", false) && !isMFRCEnabled) {
            return 0;
        }

        int32_t aidl_ret = 0, mfrc_ver = 0, wrap_ver = IGpuService::version;

        getGpuService();

        if (check_IGpuService_supported() && gGpuService != nullptr && gGpuService->getInterfaceVersion(&mfrc_ver).isOk() && mfrc_ver >= wrap_ver) {
            ALOGD("[mfrcNotifyForegroundApp] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

            ndk::ScopedAStatus ret = gGpuService->NotifyAppState(packname, actname, pid, activityId, status, uid, &aidl_ret);
        }

        if (status == STATE_DEAD) { // AMS only notifies STATE_PAUSED, STATE_RESUMED, and STATE_DEAD events to powerhal.
            ALOGI("Release GPU service pointer");
            gGpuService = nullptr;
        }

        return 0;
    }
}

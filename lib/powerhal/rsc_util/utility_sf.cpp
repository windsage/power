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
 * MediaTek Inc. (C) 2018. All rights reserved.
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
#include <utils/Log.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <aidl/vendor/mediatek/framework/mtksf_ext/IMtkSF_ext.h>
#include "utility_sf.h"
#include "common.h"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)

#define SF_HINT_LOW_POWER_ENABLED "/proc/powerhal_cpu_ctrl/sf_hint_low_power_enabled"

using ::ndk::SpAIBinder;
using ::aidl::vendor::mediatek::framework::mtksf_ext::IMtkSF_ext;

std::shared_ptr<IMtkSF_ext> gMtkSF;

int sf_dur_60 = -1;
int app_dur_60 = -1;
int sf_dur_90 = -1;
int app_dur_90 = -1;
int sf_dur_120 = -1;
int app_dur_120 = -1;
int mtk_sf_hint_low_power_enabled = 0;

int get_sf_service() {
    const char * descriptor = IMtkSF_ext::descriptor;
    if (descriptor) {
        const std::string kInstance = std::string() + descriptor + "/default";
        gMtkSF = IMtkSF_ext::fromBinder(SpAIBinder(AServiceManager_getService(kInstance.c_str())));
        if (gMtkSF != nullptr) {
            LOG_I("Loaded IMtkSF_ext service");
        } else {
            LOG_E("Cannot load IMtkSF_ext service");
        }
        return (gMtkSF != nullptr);
    } else {
        LOG_E("descriptor is null");
        return 0;
    }
}

int get_sf_low_power_hint_status() {
    return mtk_sf_hint_low_power_enabled;
}

int enable_sf_low_power_hint(int value, void *scn) {
    bool result = 0;

    if (get_sf_service() && gMtkSF != nullptr) {
        LOG_I("%d", value);
        ndk::ScopedAStatus r = gMtkSF->setPELT32(value, &result);
    }

    mtk_sf_hint_low_power_enabled = value;

    set_value(SF_HINT_LOW_POWER_ENABLED, value);

    return 0;
}

int setDuration(int app, int sf, int hz) {
    bool result = 0;

    if (get_sf_service() && gMtkSF != nullptr) {
        LOG_I("app:%d, sf:%d, hz:%d", app, sf, hz);
        ndk::ScopedAStatus r = gMtkSF->setDuration(app, sf, hz, &result);
    }

    return 0;
}

int set_sf_duration_60(int value, void *scn) {
    LOG_I("%d", value);

    sf_dur_60 = value;
    if (sf_dur_60 != -1 && app_dur_60 != -1) {
        setDuration(app_dur_60, sf_dur_60, 60);
        sf_dur_60 = -1;
        app_dur_60 = -1;
    }

    return 0;
}

int set_app_duration_60(int value, void *scn) {
    LOG_I("%d", value);

    app_dur_60 = value;
    if (sf_dur_60 != -1 && app_dur_60 != -1) {
        setDuration(app_dur_60, sf_dur_60, 60);
        sf_dur_60 = -1;
        app_dur_60 = -1;
    }

    return 0;
}

int set_sf_duration_90(int value, void *scn) {
    LOG_I("%d", value);

    sf_dur_90 = value;
    if (sf_dur_90 != -1 && app_dur_90 != -1) {
        setDuration(app_dur_90, sf_dur_90, 90);
        sf_dur_90 = -1;
        app_dur_90 = -1;
    }

    return 0;
}

int set_app_duration_90(int value, void *scn) {
    LOG_I("%d", value);

    app_dur_90 = value;
    if (sf_dur_90 != -1 && app_dur_90 != -1) {
        setDuration(app_dur_90, sf_dur_90, 90);
        sf_dur_90 = -1;
        app_dur_90 = -1;
    }

    return 0;
}

int set_sf_duration_120(int value, void *scn) {
    LOG_I("%d", value);

    sf_dur_120 = value;
    if (sf_dur_120 != -1 && app_dur_120 != -1) {
        setDuration(app_dur_120, sf_dur_120, 120);
        sf_dur_120 = -1;
        app_dur_120 = -1;
    }

    return 0;
}

int set_app_duration_120(int value, void *scn) {
    LOG_I("%d", value);

    app_dur_120 = value;
    if (sf_dur_120 != -1 && app_dur_120 != -1) {
        setDuration(app_dur_120, sf_dur_120, 120);
        sf_dur_120 = -1;
        app_dur_120 = -1;
    }

    return 0;
}

int unset_sf_duration_60(int value, void *scn) {
    LOG_I(" ");

    sf_dur_60 = 0;
    if (sf_dur_60 != -1 && app_dur_60 != -1) {
        setDuration(app_dur_60, sf_dur_60, 0);
        sf_dur_60 = -1;
        app_dur_60 = -1;
    }

    return 0;
}

int unset_app_duration_60(int value, void *scn) {
    LOG_I(" ");

    app_dur_60 = 0;
    if (sf_dur_60 != -1 && app_dur_60 != -1) {
        setDuration(app_dur_60, sf_dur_60, 0);
        sf_dur_60 = -1;
        app_dur_60 = -1;
    }

    return 0;
}

int unset_sf_duration_90(int value, void *scn) {
    LOG_I(" ");

    sf_dur_90 = 0;
    if (sf_dur_90 != -1 && app_dur_90 != -1) {
        setDuration(app_dur_90, sf_dur_90, 0);
        sf_dur_90 = -1;
        app_dur_90 = -1;
    }

    return 0;
}

int unset_app_duration_90(int value, void *scn) {
    LOG_I(" ");

    app_dur_90 = 0;
    if (sf_dur_90 != -1 && app_dur_90 != -1) {
        setDuration(app_dur_90, sf_dur_90, 0);
        sf_dur_90 = -1;
        app_dur_90 = -1;
    }

    return 0;
}

int unset_sf_duration_120(int value, void *scn) {
    LOG_I(" ");

    sf_dur_120 = 0;
    if (sf_dur_120 != -1 && app_dur_120 != -1) {
        setDuration(app_dur_120, sf_dur_120, 0);
        sf_dur_120 = -1;
        app_dur_120 = -1;
    }

    return 0;
}

int unset_app_duration_120(int value, void *scn) {
    LOG_I(" ");

    app_dur_120 = 0;
    if (sf_dur_120 != -1 && app_dur_120 != -1) {
        setDuration(app_dur_120, sf_dur_120, 0);
        sf_dur_120 = -1;
        app_dur_120 = -1;
    }

    return 0;
}

/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <aidl/vendor/mediatek/hardware/mtkpower_applist/BnMtkpower_applist.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#define LOG_TAG "MTK_APPList"
#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)

enum SysInfo {
    SYSINFO_CUR_MODE = 0,
    SYSINFO_RELOAD_APPLIST_CONFIG,
    SYSINFO_TEST_MODE,
    SYSINFO_APP_LIST_CONFIG,
    CONTROLLER_TYPE_APP_CFG,
    CONTROLLER_TYPE_THREAD_HINT,
};

namespace aidl {
namespace vendor {
namespace mediatek {
namespace hardware {
namespace mtkpower_applist {

class Mtkpower_applist : public BnMtkpower_applist {
    public:
        Mtkpower_applist();
        ndk::ScopedAStatus notifyAppState(const std::string& packName, const std::string& actName, int pid, int state, int uid) override;
        ndk::ScopedAStatus notifyAppStateInfo(const std::string& packName, const std::string& actName, int pid, int activityId, int state, int uid) override;
        ndk::ScopedAStatus setSysInfo(int cmd, const std::string& data) override;
        ndk::ScopedAStatus getSysInfo(int cmd, std::string *_aidl_ret) override;
};

}  // namespace mtkpower_applist
}  // namespace hardware
}  // namespace mediatek
}  // namespace vendor
}  // namespace aidl
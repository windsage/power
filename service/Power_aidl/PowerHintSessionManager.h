/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <utils/RefBase.h>
#include <log/log.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <mutex>
#include <optional>
#include <unordered_set>

#include "PowerHintSession.h"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

#define ADPF_MAX_SESSION   64

namespace aidl::android::hardware::power::impl::mediatek {

class PowerHintSessionManager : public virtual ::android::RefBase {
  public:
    int addPowerSession(PowerHintSession *session);
    int removePowerSession(PowerHintSession *session);
    int checkPowerSesssion(int tgid, int uid, const std::vector<int32_t>& threadIds);
    static ::android::sp<PowerHintSessionManager> getInstance() {
        static ::android::sp<PowerHintSessionManager> instance = new PowerHintSessionManager();
        return instance;
    }

  private:
    PowerHintSessionManager() {}
    int checkThreads(const std::vector<int32_t>& session_threadIds, const std::vector<int32_t>& threadIds);
    std::mutex mLock;
    std::unordered_set<PowerHintSession *> mPowerHintSessions;
    int sessionHintIds[ADPF_MAX_SESSION];
};

}  // namespace aidl::android::hardware::power::impl::mediatek

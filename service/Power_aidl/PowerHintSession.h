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

#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <aidl/android/hardware/power/SessionHint.h>
#include <aidl/android/hardware/power/WorkDuration.h>
#include <log/log.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

namespace aidl::android::hardware::power::impl::mediatek {

class PowerHintSession : public BnPowerHintSession {
  public:
    PowerHintSession(int32_t sessionId, int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos);
    ~PowerHintSession();
    ndk::ScopedAStatus updateTargetWorkDuration(int64_t targetDurationNanos) override;
    ndk::ScopedAStatus reportActualWorkDuration(const std::vector<WorkDuration>& durations) override;
    ndk::ScopedAStatus pause() override;
    ndk::ScopedAStatus resume() override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus sendHint(SessionHint hint) override;
    ndk::ScopedAStatus setThreads(const std::vector<int32_t>& threadIds) override;
    int getSessionId();
    int getTgid();
    int getUid();
    std::vector<int32_t> getThreadIds();
    int getDurationNanos();

  private:
    int id;
    int tgid;
    int uid;
    std::vector<int32_t> threadIds;
    int durationNanos;
};

}  // namespace aidl::android::hardware::power::impl::mediatek

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

#include <aidl/vendor/mediatek/hardware/mtkpower/BnMtkPowerService.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

namespace aidl {
namespace vendor {
namespace mediatek {
namespace hardware {
namespace mtkpower {

class MtkPowerService : public BnMtkPowerService {
    public:
        MtkPowerService();
        ~MtkPowerService();
        ndk::ScopedAStatus perfLockAcquire(int hdl, int duration, const std::vector<int>& boostList, int pid, int reserved, int* _aidl_return) override;
        ndk::ScopedAStatus perfLockRelease(int hdl, int reserved) override;
        ndk::ScopedAStatus perfLockReleaseSync(int hdl, int reserved, int* _aidl_return) override;
        ndk::ScopedAStatus perfCusLockHint(int hint, int duration, int pid, int* _aidl_return) override;
        ndk::ScopedAStatus mtkPowerHint(int hint, int data)  override;
        ndk::ScopedAStatus mtkCusPowerHint(int hint, int data)  override;
        ndk::ScopedAStatus querySysInfo(int cmd, int param, int* _aidl_return) override;
        ndk::ScopedAStatus setSysInfo(int type, const std::string& data, int* _aidl_return) override;
        ndk::ScopedAStatus setSysInfoAsync(int type, const std::string& data)  override;
        ndk::ScopedAStatus setMtkPowerCallback(const std::shared_ptr<::aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback, int* _aidl_return) override;
        ndk::ScopedAStatus setMtkScnUpdateCallback(int scn, const std::shared_ptr<::aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback, int* _aidl_return) override;
};

}  // namespace mtkpower
}  // namespace hardware
}  // namespace mediatek
}  // namespace vendor
}  // namespace aidl

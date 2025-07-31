/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "mtkpower-service.mediatek"

#include <hidl/LegacySupport.h>
#include <unistd.h>
#include <pthread.h>
#include "PowerManager.h"
#include "libpowerhal_wrap.h"
#include "MtkPowerService.h"
#include "libpowercloud.h"
#include "Power.h"
#include "power_timer.h"
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::power::impl::mediatek::Power;

using ::android::hardware::registerPassthroughServiceImplementation;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::OK;
using ::android::status_t;

pthread_mutex_t g_mutex;
pthread_cond_t  g_cond;
bool powerd_done = false;

void* IPower_IMtkPower_Service(void *data)
{
    ALOGI("Start AIDL android.hardware.power.IPower");

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Power> vib_power = ndk::SharedRefBase::make<Power>();

    const std::string instance_power = std::string() + Power::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vib_power->asBinder().get(), instance_power.c_str());
    if (status != STATUS_OK) {
        ALOGE("IPower - add service fail:%d", (int)status);
        return NULL;
    }

    ALOGI("Start AIDL vendor.mediatek.hardware.mtkpower.IMtkPowerService");

    std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::MtkPowerService> vib_mtkpower = ndk::SharedRefBase::make<aidl::vendor::mediatek::hardware::mtkpower::MtkPowerService>();
    const std::string instance_mtkpower = std::string() + aidl::vendor::mediatek::hardware::mtkpower::MtkPowerService::descriptor + "/default";
    status = AServiceManager_addService(vib_mtkpower->asBinder().get(), instance_mtkpower.c_str());
    if (status != STATUS_OK) {
        ALOGE("IMtkPowerService - add service fail:%d", (int)status);
        return NULL;
    }

    ALOGI("IPower & IMtkPowerService joinRpcThreadpool");

    ABinderProcess_joinThreadPool();
    return NULL;
}


int main() {
    pthread_t serviceAidlThread;
    pthread_t timerThread;
    pthread_attr_t attr;

    //libpowerhal_wrap_Init();
    //SPD: add cloudEnigne for powerhal by fan.feng1 at 20220916 start
    registCloudctlListener();
    //SPD: add cloudEnigne for powerhal by fan.feng1 at 20220916 end
    /* service */
    pthread_attr_init(&attr);
    pthread_create(&serviceAidlThread, &attr, IPower_IMtkPower_Service, (void*)&serviceAidlThread);
    pthread_setname_np(serviceAidlThread, "AIDL_IPower_IMtkPower");

    createTimerThread(&timerThread);

    pthread_join(serviceAidlThread, NULL);
    pthread_join(timerThread, NULL);
    //SPD: add cloudEnigne for powerhal by fan.feng1 at 20220916 start
    unregistCloudctlListener();
    //SPD: add cloudEnigne for powerhal by fan.feng1 at 20220916 end
    return 0;
}


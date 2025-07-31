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

#define LOG_TAG "libPowerHal-bt"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <utils/Log.h>
#include <errno.h>

#include "utility_consys_bt.h"
#include "perfservice.h"
#include <future>
#include <vendor/mediatek/hardware/bluetooth/audio/2.2/IBluetoothAudioProvidersFactory.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProviderFactory.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <cutils/properties.h>
#include <android/binder_manager.h>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
using namespace std::chrono_literals;

using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::vendor::mediatek::hardware::bluetooth::audio::V2_2::IBluetoothAudioProvidersFactory;
using ::vendor::mediatek::hardware::bluetooth::audio::V2_2::IBluetoothAudioProvider;
using ::android::sp;
using ::android::hardware::hidl_death_recipient;
using ::android::hidl::base::V1_0::IBase;
using ::android::wp;
using ::vendor::mediatek::hardware::bluetooth::audio::V2_2::SessionType;
using AidlSessionType =
    ::aidl::android::hardware::bluetooth::audio::SessionType;
using BluetoothAudioStatus =
      ::vendor::mediatek::hardware::bluetooth::audio::V2_1::Status;
using AidlBluetoothAudioProvider =
    ::aidl::android::hardware::bluetooth::audio::IBluetoothAudioProvider;
using AidlBluetoothAudioProviderFactory =
    ::aidl::android::hardware::bluetooth::audio::IBluetoothAudioProviderFactory;
android::sp<IBluetoothAudioProvider> bluetooth_audio_provider = nullptr;
std::shared_ptr<AidlBluetoothAudioProvider> aidl_a2dp_offload_provider = nullptr;
using namespace std;

static inline const std::string kDefaultAudioProviderFactoryInterface =
        std::string() + AidlBluetoothAudioProviderFactory::descriptor + "/default";
::ndk::ScopedAIBinder_DeathRecipient aidl_death_recipient;
//support_aidl_status: -1, not init, 0, not support, 1, support
int support_aidl_status = -1;
/**
 * Public API for bluetooth a2dp low lentency
 */

class BluetoothGameModeTask {
    using EnterFunc = std::function<void(int)>;
    public:
        BluetoothGameModeTask(EnterFunc func) {
            mEnterFunc = func;
            mCurrentMode = -1;
            mGameQu.clear();
            mThread = std::thread(&BluetoothGameModeTask::run, this);
        }
        ~BluetoothGameModeTask() {
            //when this instance created, will not be destroyed
            mGameQu.clear();
            mThread.join();
        }
        void Reset() {
            ALOGI("%s: reset", __func__);
            std::lock_guard<std::mutex> lock(mLock);
            mGameQu.clear();
        }
        void EnterGameMode(int mode) {
            ALOGI("%s: mode = %d", __func__, mode);
            std::lock_guard<std::mutex> lock(mLock);
            if (mode == mCurrentMode) {
                //clear the mode pendding in the queue
                ALOGI("%s: is same as current mode, clear all pending mode", __func__);
                mGameQu.clear();
                return;
            }
            int lastMode = -1;
            if (!mGameQu.empty())
                lastMode = mGameQu.back();
            if (lastMode != mode) {
                ALOGI("%s: last mode %d, set mode = %d, put in queue",__func__,lastMode,mCurrentMode);
                mGameQu.push_back(mode);
                //notify task to enter game mode
                mCv.notify_all();
            }
        }
    private:
        void run() {
            ALOGI("%s", __func__);
            while(true) {
                {
                    //wait mode
                    std::unique_lock<std::mutex> lock(mLock);
                    mCv.wait(lock, [&]{return!mGameQu.empty();});
                }
                while(true) {
                    {
                        std::lock_guard<std::mutex> lock(mLock);
                        if (mGameQu.empty()) break;
                        mCurrentMode = mGameQu.front();
                        mGameQu.erase(mGameQu.begin());
                        ALOGI("%s: call enter function with mode %d",__func__,mCurrentMode);
                    }
                    mEnterFunc(mCurrentMode);
                    ALOGI("%s: call enter function end",__func__);
                }
            }
        }
    private:
        EnterFunc mEnterFunc;
        std::vector<int> mGameQu;
        std::mutex mLock;
        int mCurrentMode;
        std::thread mThread;
        std::condition_variable mCv;
};

static BluetoothGameModeTask *bt_game_mode_task = nullptr;
class BluetoothAudioDeathRecipient
    : public ::android::hardware::hidl_death_recipient {

public:
    void serviceDied(
         uint64_t /*cookie*/,
         const ::android::wp<::android::hidl::base::V1_0::IBase> & /*who*/) {
        ALOGE("bluetooth provider is death");
        if (bt_game_mode_task)
            bt_game_mode_task->Reset();
        bluetooth_audio_provider = nullptr;
    }
};
android::sp<BluetoothAudioDeathRecipient> death_recipicent;


static int get_bluetooth_audio_provider()
{
    android::sp<IBluetoothAudioProvidersFactory> providersFactory =
             IBluetoothAudioProvidersFactory::getService();
    if (providersFactory == nullptr) {
       ALOGE("bluetooth provider factory service get fail");
       return 0;
    }
    std::promise<void> open_promise;
    auto open_future = open_promise.get_future();
    auto open_cb = [&open_promise](
                      BluetoothAudioStatus status,
                      const android::sp<IBluetoothAudioProvider>& provider) {
        ALOGI("open provider cb");
        if (status == BluetoothAudioStatus::SUCCESS) {
           bluetooth_audio_provider = provider;
        }
        if (bluetooth_audio_provider == nullptr) {
           ALOGE("failed to open provider");
        }
        open_promise.set_value();
    };
    auto hidl_retval = providersFactory->openProvider_2_1(
                                  SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,open_cb);
    open_future.get();
    if (!hidl_retval.isOk() || bluetooth_audio_provider == nullptr) {
      ALOGE("fail to open bluetooth provider");
      return 0;
    }
    if (!bluetooth_audio_provider->linkToDeath(death_recipicent,0).isOk()) {
      ALOGE("linkToDeath fail");
      bluetooth_audio_provider = nullptr;
      return 0;
    }
    ALOGI("get provider successfully");
    return 1;
}

void AidlBinderDiedCallback(void* ptr) {
    ALOGE("Bluetooth audio aidl died");
    if (bt_game_mode_task)
        bt_game_mode_task->Reset();
    aidl_a2dp_offload_provider = nullptr;
}

static int get_aidl_provider()
{
    if (support_aidl_status == -1) {
        support_aidl_status = 0;
        if (AServiceManager_checkService(
            kDefaultAudioProviderFactoryInterface.c_str()) != nullptr) {
            support_aidl_status = 1;
            ALOGI("aidl a2dp offload provider support\n");
        }
        else {
            ALOGE("aidl a2dp offload provider not support\n");
        }
    }
    if (support_aidl_status != 1) {
        aidl_a2dp_offload_provider = nullptr;
        return 0;
    }
    auto provider_factory = AidlBluetoothAudioProviderFactory::fromBinder(
        ::ndk::SpAIBinder(AServiceManager_getService(
            kDefaultAudioProviderFactoryInterface.c_str()))
    );
    if (provider_factory == nullptr) {
        ALOGW("open aidl blueooth audiofactory fail\n");
        return 0;
    }
    auto aidl_ret = provider_factory->openProvider(AidlSessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
        &aidl_a2dp_offload_provider);
    if (!aidl_ret.isOk()) {
        ALOGW("Open aidl a2dp offload provider fail\n");
        return 0;
    }
    aidl_death_recipient = ::ndk::ScopedAIBinder_DeathRecipient(
        AIBinder_DeathRecipient_new(AidlBinderDiedCallback));
    AIBinder_linkToDeath(provider_factory->asBinder().get(),
        aidl_death_recipient.get(), nullptr);
    return 1;
}


static void set_performace_prop(int enable, bool is_ultra)
{
    const char* normal_game_prop_value = enable > 0 ? "true" : "false";
    const char* ultra_game_prop_value = (is_ultra && enable > 0) ? "true" : "false";
    ALOGI("%s: will set prop %s=%s ; %s=%s",__func__,kBluetoothGameMode,normal_game_prop_value,
          kBluetoothUltraGameMode,ultra_game_prop_value);
    int ret = property_set(kBluetoothGameMode, normal_game_prop_value);
    if (ret != 0) {
        ALOGI("bt a2dp low latency set property fail");
    }
    ret = property_set(kBluetoothUltraGameMode, ultra_game_prop_value);
    if (ret != 0) {
        ALOGI("bt a2dp low latency set ultra property fail");
    }
}

int notify_to_bluetooth_audio_hal(int enable, void *data)
{
    if (!aidl_a2dp_offload_provider) {
        get_aidl_provider();
    }
    if (aidl_a2dp_offload_provider) {
        auto aidl_ret = aidl_a2dp_offload_provider->setLowLatencyModeAllowed(enable);
        if (!aidl_ret.isOk()) {
            ALOGE("bluetooth audio provider is death.");
            aidl_a2dp_offload_provider = nullptr;
        }
    }
    //if not support aidl, used hidl
    if (!bluetooth_audio_provider) {
        get_bluetooth_audio_provider();
    }
    if (bluetooth_audio_provider) {
        auto hidl_retval = bluetooth_audio_provider->enterGameMode(enable);
        if (!hidl_retval.isOk()) {
            ALOGE("bluetooth audio provider is death.");
            bluetooth_audio_provider = nullptr;
        }
    }
    return 0;
}

int bt_audio_low_latency(int enable, void *data)
{
    ALOGI("%s: enter = %d, data:%p",__func__, enable, data);
    //set prop
    set_performace_prop(enable, false);
    if (!bt_game_mode_task) {
        bt_game_mode_task = new BluetoothGameModeTask(std::bind(
                                notify_to_bluetooth_audio_hal,std::placeholders::_1,nullptr));
    }
    bt_game_mode_task->EnterGameMode(enable);
    ALOGI("%s: done!", __func__);
    return 0;
}

int bt_audio_ultra_low_latency(int enable, void *data)
{
    ALOGI("%s enter = %d, data:%p",__func__, enable, data);
    //set prop
    set_performace_prop(enable, true);
    if (!bt_game_mode_task) {
        bt_game_mode_task = new BluetoothGameModeTask(std::bind(
                                notify_to_bluetooth_audio_hal,std::placeholders::_1,nullptr));
    }
    bt_game_mode_task->EnterGameMode(enable);
    ALOGI("%s: done!", __func__);
    return 0;
}


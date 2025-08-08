#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define LOG_TAG "mtkpower_client"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <log/log.h>
#include <inttypes.h>
#include <vector>
#include <pthread.h>
#include <cutils/properties.h>
#include <aidl/vendor/mediatek/hardware/mtkpower/IMtkPowerService.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <vendor/mediatek/hardware/mtkpower/1.1/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPerf.h>

using android::hardware::Return;
using android::hardware::hidl_death_recipient;
using android::hidl::base::V1_0::IBase;
using std::vector;
using ::android::hardware::hidl_vec;
using IMtkPerfV1_0 = vendor::mediatek::hardware::mtkpower::V1_0::IMtkPerf;
using IMtkPerfV1_1 = vendor::mediatek::hardware::mtkpower::V1_1::IMtkPerf;
using IMtkPerfV1_2 = vendor::mediatek::hardware::mtkpower::V1_2::IMtkPerf;
using ::ndk::SpAIBinder;
static bool gIMtkPowerServiceExists = true;
static int IMtkPowerService_supported = -1;
std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService> gIMtkPowerService;
static bool gMtkPerfHalExists = true;
static android::sp<IMtkPerfV1_0> gMtkPerfHalV1_0 = nullptr;
static android::sp<IMtkPerfV1_1> gMtkPerfHalV1_1 = nullptr;
static android::sp<IMtkPerfV1_2> gMtkPerfHalV1_2 = nullptr;
static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
static bool getMtkPerfHal();

static bool getIMtkPowerService() {
    const char * descriptor = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::descriptor;
    if (descriptor) {
        const std::string instance = std::string() + descriptor + "/default";
        if (gIMtkPowerService == nullptr) {
            gIMtkPowerService = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::fromBinder(SpAIBinder(AServiceManager_getService(instance.c_str())));

            if (gIMtkPowerService != nullptr) {
                ALOGI("load IMtkPowerService successfully");
            } else {
                ALOGE("cannot load IMtkPowerService");
                gIMtkPowerServiceExists = false;
            }
        }
        return (gIMtkPowerService != nullptr);
    } else {
        ALOGE("descriptor is null");
        return false;
    }
}

static int check_IMtkPowerService_supported()
{
    const char * descriptor = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::descriptor;

    if(IMtkPowerService_supported != -1)
        return IMtkPowerService_supported;

    if (descriptor) {
        static const std::string IMtkPowerService_instance = std::string() + descriptor + "/default";
        IMtkPowerService_supported = AServiceManager_isDeclared(IMtkPowerService_instance.c_str());
        ALOGI("IMtkPowerService_isDeclared %d", IMtkPowerService_supported);
        return IMtkPowerService_supported;
    } else {
        ALOGE("descriptor is null");
        return 0;
    }
}

static bool getMtkPerfHal() {
    if (gMtkPerfHalV1_0 == nullptr) {
        gMtkPerfHalV1_0 = IMtkPerfV1_0::getService();
        if (gMtkPerfHalV1_0 != nullptr) {
            ALOGI("Loaded mtkperf HAL 1.0 service");

            gMtkPerfHalV1_1 = IMtkPerfV1_1::castFrom(gMtkPerfHalV1_0);
            if (gMtkPerfHalV1_1 != nullptr) {
                ALOGI("Loaded mtkperf HAL 1.1 service");

                    gMtkPerfHalV1_2 = IMtkPerfV1_2::castFrom(gMtkPerfHalV1_1);
                    if (gMtkPerfHalV1_2 != nullptr)
                        ALOGI("Loaded mtkperf HAL 1.2 service");
            }
        } else {
            ALOGI("Couldn't load power HAL service");
            gMtkPerfHalExists = false;
        }
    }

    if (gMtkPerfHalV1_2 == nullptr) {
        ALOGE("[getMtkPerfHal] nullptr");
        return false;
    }

    return true;
}

static void processReturn(const Return<void> &ret, const char* functionName) {
    if(!ret.isOk()) {
        ALOGE("%s() failed: Power HAL service not available", functionName);
        gMtkPerfHalV1_0 = nullptr;
        gMtkPerfHalV1_1 = nullptr;
        gMtkPerfHalV1_2 = nullptr;
    }
}

static int processReturnWithInt32(const Return<int32_t> &ret, const char* functionName) {
    if(!ret.isOk()) {
        ALOGE("%s() failed: Power HAL service not available", functionName);
        gMtkPerfHalV1_0 = nullptr;
        gMtkPerfHalV1_1 = nullptr;
        gMtkPerfHalV1_2 = nullptr;
        return 0;
    } else {
        return 1;
    }
}

extern "C"
int perf_lock_acq(int hdl, int duration, int list[], int numArgs)
{
    int my_pid = (int)getpid();
    int my_tid = (int)gettid();
    std::vector<int32_t> rscList;
    int i, ret_hdl = 0;

    /* check input parameter */
    if(numArgs % 2 != 0) {
        ALOGE("perf_lock_acq numArgs is wrong");
        return -1;
    }

    ALOGI("%s, hdl:%d, dur:%d, num:%d, pid:%d, tid:%d", __FUNCTION__, hdl, duration, numArgs, my_pid, my_tid);

    /* log */
    for (i=0; i<numArgs; i+=2) {
        ALOGD("[perf_lock_acq] list:0x%08x, %d", list[i], list[i+1]);
    }

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            rscList.assign(list, (list+numArgs));
            ndk::ScopedAStatus ret = gIMtkPowerService->perfLockAcquire(hdl, duration, rscList, my_pid, my_tid, &ret_hdl);
            ALOGI("ret_hdl:%d",ret_hdl);
        }
    } else {
        if (getMtkPerfHal()) {
            rscList.assign(list, (list+numArgs));
            Return<int32_t> ret = gMtkPerfHalV1_2->perfLockAcquire(hdl, duration, rscList, my_tid);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                ret_hdl = ret;
                ALOGI("ret_hdl:%d",ret_hdl);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return ret_hdl;
}

extern "C"
int perf_lock_rel(int hdl)
{
    int my_pid = (int)getpid();
    int my_tid = (int)gettid();
    int r = 0;

    if (hdl <= 0) {
        ALOGD("%s, hdl:%d, pid:%d, tid:%d, skip",__FUNCTION__, hdl, my_pid, my_tid);
        return 0;
    }

    ALOGI("%s, hdl:%d, pid: %d, tid:%d",__FUNCTION__, hdl, my_pid, my_tid);

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->perfLockReleaseSync(hdl, my_pid, &r);
        }
    } else {
        if (getMtkPerfHal()) {
            Return<int32_t> ret = gMtkPerfHalV1_2->perfLockReleaseSync(hdl, my_tid);
            processReturnWithInt32(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int perf_lock_rel_async(int hdl)
{
    int my_pid = (int)getpid();
    int my_tid = (int)gettid();

    if (hdl <= 0) {
        ALOGD("%s, hdl:%d, pid:%d, tid:%d, skip",__FUNCTION__, hdl, my_pid, my_tid);
        return 0;
    }

    ALOGI("%s, hdl:%d, pid:%d, tid:%d",__FUNCTION__, hdl, my_pid, my_tid);

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->perfLockRelease(hdl, my_pid);
        }
    } else {
        if (getMtkPerfHal()) {
            Return<void> ret = gMtkPerfHalV1_2->perfLockRelease(hdl, my_tid);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int perf_cus_lock_hint(int hint, int duration)
{
    int my_pid = (int)getpid();
    int ret_hdl = 0;

    ALOGI("%s hint:%d, dur:%d, pid:%d", __FUNCTION__, hint, duration, my_pid);

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->perfCusLockHint(hint, duration, my_pid, &ret_hdl);
            ALOGI("%s, ret_hdl:%d", __FUNCTION__, ret_hdl);
        }
    } else {
        if (getMtkPerfHal() && gMtkPerfHalV1_2 != nullptr) {
            Return<int32_t> ret = gMtkPerfHalV1_2->perfCusLockHint(hint, duration);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                ret_hdl = ret;
                ALOGI("%s, ret_hdl:%d", __FUNCTION__, ret_hdl);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return ret_hdl;
}

//} // namespace


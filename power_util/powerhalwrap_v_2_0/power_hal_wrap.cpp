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
#include <dlfcn.h>
#include <unistd.h>
#include <log/log.h>
#include <inttypes.h>
#include <vector>
#include <cutils/properties.h>
#include <aidl/vendor/mediatek/hardware/mtkpower/IMtkPowerService.h>
#include <aidl/vendor/mediatek/hardware/mtkpower/ThreadHintParams.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.0/IMtkPower.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPerf.h>
#include <aidl/vendor/mediatek/hardware/mtkpower_applist/IMtkpower_applist.h>

#include <mtkperf_resource.h>
#include <common.h>
#include "util/utility_applist.h"
#include "util/utility_gcn.h"
#include "util/utility_magt.h"
#include "util/utility_mfrc.h"
#include "util/utility_task_turbo.h"

//#include "mtkpower_hint.h" // cannot include vendor header file

using android::hardware::Return;
using android::hardware::hidl_death_recipient;
using android::hidl::base::V1_0::IBase;
using namespace vendor::mediatek::hardware::mtkpower::V1_0;
using IMtkPerfV1_2 = vendor::mediatek::hardware::mtkpower::V1_2::IMtkPerf;
using std::vector;
using ::ndk::SpAIBinder;
using aidl::vendor::mediatek::hardware::mtkpower::ThreadHintParams;
using aidl::vendor::mediatek::hardware::mtkpower_applist::IMtkpower_applist;
static bool gIMtkPowerServiceExists = true;
static int IMtkPowerService_supported = -1;
std::shared_ptr<aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService> gIMtkPowerService;
#define BOOT_INFO_FILE "/sys/class/BOOT/BOOT/boot/boot_mode"
#define MODE_PRIORITY (6)
static bool gMtkPowerHalExists = true;
static bool gMtkPerfHalExists = true;
static android::sp<IMtkPerfV1_2> gMtkPerfHal = nullptr;
static android::sp<IMtkPower> gMtkPowerHal = nullptr;

//
static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
static bool getMtkPowerHal();
static int dfrc_fps = 60;
static int EnableMultiDisplayMode = 0;
static int CurMultiDisplayMode = -1;
static int camera_mode_on = 0;
static int camera_mode_handle = 0;
static int mtkpowerServiceVersion = 0;

// Multi games for game mode
#define GAME_MAX_NUM  16
static int gameModePidList[GAME_MAX_NUM];
static int gameModeHandle[GAME_MAX_NUM];
static int gameScnHintHandle[GAME_MAX_NUM];

/* CAUSION: should be sync with mtkpower_hint.h */
enum {
    MTKPOWER_HINT_BASE = 20,

    MTKPOWER_HINT_PROCESS_CREATE                    = 21,
    MTKPOWER_HINT_PACK_SWITCH                       = 22,
    MTKPOWER_HINT_ACT_SWITCH                        = 23,
    MTKPOWER_HINT_APP_ROTATE                        = 24,
    MTKPOWER_HINT_APP_TOUCH                         = 25,
    MTKPOWER_HINT_GALLERY_BOOST                     = 26,
    MTKPOWER_HINT_GALLERY_STEREO_BOOST              = 27,
    MTKPOWER_HINT_WFD                               = 28,
    MTKPOWER_HINT_PMS_INSTALL                       = 29,
    MTKPOWER_HINT_EXT_LAUNCH                        = 30,
    MTKPOWER_HINT_WHITELIST_LAUNCH                  = 31,
    MTKPOWER_HINT_WIPHY_SPEED_DL                    = 32,
    MTKPOWER_HINT_SDN                               = 33,
    MTKPOWER_HINT_WHITELIST_ACT_SWITCH              = 34,
    MTKPOWER_HINT_BOOT                              = 35,
    MTKPOWER_HINT_AUDIO_LATENCY_DL                  = 36,
    MTKPOWER_HINT_AUDIO_LATENCY_UL                  = 37,
    MTKPOWER_HINT_AUDIO_POWER_DL                    = 38,
    MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE     = 39,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60     = 40,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90     = 41,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120    = 42,
    MTKPOWER_HINT_UX_SCROLLING                      = 43,
    MTKPOWER_HINT_AUDIO_POWER_UL                    = 44,
    MTKPOWER_HINT_UX_MOVE_SCROLLING                 = 45,
    MTKPOWER_HINT_AUDIO_POWER_HI_RES                = 46,
    MTKPOWER_HINT_GAME_MODE                         = 47,
    MTKPOWER_HINT_VIDEO_MODE                        = 48,
    MTKPOWER_HINT_UX_SCROLLING_COMMON               = 49,
    MTKPOWER_HINT_FRS_ACT                           = 50,
    MTKPOWER_HINT_CAMERA_MODE                       = 51,
    MTKPOWER_HINT_SF_LOW_POWER_MODE                 = 52,
    MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE_GAME     = 54,
    MTKPOWER_HINT_EXT_LAUNCH_FOR_GAME               = 55,
    MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE          = 57,
    MTKPOWER_HINT_PACK_SWITCH_ANIMATION_OFF         = 58,
    MTKPOWER_HINT_EXT_ACT_SWITCH                    = 59,
    MTKPOWER_HINT_EXT_LAUNCH_ANIMATION_OFF          = 60,
    MTKPOWER_HINT_AUDIO_ROUTING                     = 62,
    MTKPOWER_HINT_PROCESS_CREATE_BALANCE_MODE_GAME  = 63,
    MTKPOWER_HINT_HOT_LAUNCH                        = 64,

    MTKPOWER_HINT_NUM,
};

enum {
    NORMAL_MODE = 0,
    GAME_MODE,
    MODE_COUNT,
};

enum {
    MTKPOWER_CMD_GET_CLUSTER_NUM            = 1,
    MTKPOWER_CMD_GET_CLUSTER_CPU_NUM        = 2,
    MTKPOWER_CMD_GET_CLUSTER_CPU_FREQ_MIN   = 3,
    MTKPOWER_CMD_GET_CLUSTER_CPU_FREQ_MAX   = 4,
    MTKPOWER_CMD_GET_GPU_FREQ_COUNT         = 5,
    MTKPOWER_CMD_GET_FOREGROUND_PID         = 6,
    MTKPOWER_CMD_GET_FOREGROUND_TYPE        = 7,
    MTKPOWER_CMD_GET_FOREGROUND_UID         = 8,
    MTKPOWER_CMD_GET_PROCESS_CREATE_STATUS  = 9,

    MTKPOWER_CMD_GET_CPU_TOPOLOGY           = 20,
    MTKPOWER_CMD_GET_LOAD_TRACKING          = 21,

    MTKPOWER_CMD_GET_RILD_CAP               = 40,
    MTKPOWER_CMD_GET_TIME_TO_LAST_TOUCH     = 41,
    MTKPOWER_CMD_GET_RES_SETTING            = 42,

    MTKPOWER_CMD_GET_LAUNCH_TIME_COLD              = 100,
    MTKPOWER_CMD_GET_LAUNCH_TIME_WARM              = 101,
    MTKPOWER_CMD_GET_POWER_HINT_HOLD_TIME          = 102,
    MTKPOWER_CMD_GET_POWER_HINT_EXT_HINT           = 103,
    MTKPOWER_CMD_GET_POWER_HINT_EXT_HINT_HOLD_TIME = 104,
    MTKPOWER_CMD_GET_POWER_SCN_TYPE                = 105,
    MTKPOWER_CMD_GET_ACT_SWITCH_TIME               = 106,
    MTKPOWER_CMD_GET_POWER_HDL_HOLD_TIME           = 107,
    MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT            = 108,
    MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT_HOLD_TIME  = 109,
    MTKPOWER_CMD_GET_INSTALL_MAX_DURATION          = 110,

    MTKPOWER_CMD_GET_DEBUG_SET_LVL           = 200,
    MTKPOWER_CMD_GET_DEBUG_DUMP_ALL          = 201,
    MTKPOWER_CMD_GET_POWERHAL_ONOFF          = 202,
    MTKPOWER_CMD_GET_RES_CTRL_ON             = 203,
    MTKPOWER_CMD_GET_RES_CTRL_OFF            = 204,
    MTKPOWER_CMD_GET_API_STATUS              = 205,
    MTKPOWER_CMD_GET_FPS_VALUE               = 206,
    MTKPOWER_CMD_GET_TOUCH_BOOST_STATUS      = 207,
    MTKPOWER_CMD_GET_DEBUG_DUMP_APP_CFG      = 208,
    MTKPOWER_CMD_GET_POWERHAL_MODE_STATUS    = 209,
    MTKPOWER_CMD_GET_INTERACTIVE_STATE       = 210,
    MTKPOWER_CMD_GET_GAME_MODE_STATUS        = 211,
};

enum {
    SETSYS_MANAGEMENT_PREDICT  = 1,
    SETSYS_SPORTS_APK          = 2,
    SETSYS_FOREGROUND_SPORTS   = 3,
    SETSYS_MANAGEMENT_PERIODIC = 4,
    SETSYS_INTERNET_STATUS     = 5,
    SETSYS_NETD_STATUS         = 6,
    SETSYS_PREDICT_INFO        = 7,
    SETSYS_NETD_DUPLICATE_PACKET_LINK = 8,
    SETSYS_PACKAGE_VERSION_NAME = 9,
    SETSYS_RELOAD_WHITELIST    = 10,
    SETSYS_POWERHAL_UNIT_TEST  = 11,
    SETSYS_API_ENABLED         = 12,
    SETSYS_API_DISABLED        = 13,
    SETSYS_FPS_VALUE           = 14,
    SETSYS_NETD_SET_FASTPATH_BY_UID = 15,
    SETSYS_NETD_SET_FASTPATH_BY_LINKINFO = 16,
    SETSYS_NETD_CLEAR_FASTPATH_RULES = 17,
    SETSYS_NETD_BOOSTER_CONFIG = 18,
    SETSYS_MULTI_WINDOW_STATUS = 19,
    SETSYS_GAME_MODE_PID       = 20,
    SETSYS_VIDEO_MODE_PID      = 21,
    SETSYS_NETD_SET_BOOST_UID  = 22,
    SETSYS_POWERHAL_GAME_MODE_ENABLED = 23,
    SETSYS_FOREGROUND_APP_PID  = 24,
    SETSYS_CAMERA_MODE_PID     = 25,
};

enum{
    MTKPOWER_HINT_ALWAYS_ENABLE = 0x0FFFFFFF,
};

static int getIMtkPowerService() {
    const char * descriptor = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::descriptor;
    if (descriptor) {
        const std::string instance = std::string() + descriptor + "/default";
        if (gIMtkPowerService == nullptr) {
            gIMtkPowerService = aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerService::fromBinder(SpAIBinder(AServiceManager_getService(instance.c_str())));
            if (gIMtkPowerService != nullptr) {
                gIMtkPowerService->getInterfaceVersion(&mtkpowerServiceVersion);
                ALOGD("power aidl version: %d", mtkpowerServiceVersion);
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

static bool getMtkPerfHal() {
    if (gMtkPerfHal == nullptr) {
        gMtkPerfHal = IMtkPerfV1_2::getService();
        if (gMtkPerfHal != nullptr) {
            ALOGI("load IMtkPerf");
        } else {
            ALOGI("cannot load IMtkPerf");
            gMtkPerfHalExists = false;
        }
    }

    if (gMtkPerfHal == nullptr) {
        ALOGE("[getMtkPerfHal] nullptr");
        return false;
    }

    return true;
}

static void processReturn(const Return<void> &ret, const char* functionName) {
    if(!ret.isOk()) {
        ALOGE("%s() failed: Power HAL service not available", functionName);
        gMtkPowerHal = nullptr;
    }
}

static int processReturnWithInt32(const Return<int32_t> &ret, const char* functionName) {
    if(!ret.isOk()) {
        ALOGE("%s() failed: Power HAL service not available", functionName);
        gMtkPowerHal = nullptr;
        return 0;
    } else {
        return 1;
    }
}

extern "C"
int PowerHal_Wrap_mtkPowerHint(uint32_t hint, int32_t data)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->mtkPowerHint(hint, data);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s, hint:%d, data:%d",__FUNCTION__, hint, data);
            Return<void> ret = gMtkPowerHal->mtkPowerHint(hint, data);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_mtkCusPowerHint(uint32_t hint, int32_t data)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(hint, data);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s, hint:%d, data:%d",__FUNCTION__, hint, data);
            Return<void> ret = gMtkPowerHal->mtkCusPowerHint(hint, data);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_RoutingBoost(int32_t data)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(MTKPOWER_HINT_AUDIO_ROUTING, data);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s, hint: MTKPOWER_HINT_AUDIO_ROUTING, data:%d",__FUNCTION__, data);
            Return<void> ret = gMtkPowerHal->mtkCusPowerHint(MTKPOWER_HINT_AUDIO_ROUTING, data);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);

    return 0;
}

extern "C"
int PowerHal_Wrap_querySysInfo(uint32_t cmd, int32_t param)
{
    int data = 0;

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->querySysInfo(cmd, param, &data);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s",__FUNCTION__);
            Return<int32_t> ret = gMtkPowerHal->querySysInfo(cmd, param);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                data = ret;
                ALOGI("%s, data:%d", __FUNCTION__, data);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return data;
}

extern "C"
int PowerHal_Wrap_notifyAppState(const char *packname, const char *actname, uint32_t pid, uint32_t activityId, int32_t status, uint32_t uid)
{
    pthread_mutex_lock(&sMutex);

    ALOGI("[PowerHal_Wrap_notifyAppState] %s/%s pid=%d activityId:%d state:%d", packname, actname, pid, activityId, status);

    applistNotifyForegroundApp(packname, actname, pid, activityId, status, uid);
    powerNotifyForegroundApp(packname, actname, pid, activityId, status, uid);
    magtNotifyForegroundApp(packname, actname, pid, activityId, status, uid);
    taskTurboNotifyForegroundApp(packname, actname, pid, activityId, status, uid);
    mfrcNotifyForegroundApp(packname, actname, pid, activityId, status, uid);

    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_scnReg(void)
{
    ALOGE("%s not support!!!",__FUNCTION__);
    return -1;
}

extern "C"
int PowerHal_Wrap_scnConfig(int32_t hdl, int32_t cmd, int32_t param1, int32_t param2, int32_t param3, int32_t param4)
{
    ALOGV("%d,%d,%d,%d,%d,%d", hdl, cmd, param1, param2, param3, param4);
    ALOGE("%s not support!!!",__FUNCTION__);
    return 0;
}

extern "C"
int PowerHal_Wrap_scnUnreg(int32_t hdl)
{
    ALOGV("%d", hdl);
    ALOGE("%s not support!!!",__FUNCTION__);
    return 0;
}

extern "C"
int PowerHal_Wrap_scnEnable(int32_t hdl, int32_t timeout)
{
    ALOGV("%d,%d", hdl, timeout);
    ALOGE("%s not support!!!",__FUNCTION__);
    return 0;
}

extern "C"
int PowerHal_Wrap_scnDisable(int32_t hdl)
{
    ALOGV("%d", hdl);
    ALOGE("%s not support!!!",__FUNCTION__);
    return 0;
}

extern "C"
int PowerHal_Wrap_scnUltraCfg(int32_t hdl, int32_t ultracmd, int32_t param1,
                                                     int32_t param2, int32_t param3, int32_t param4)
{
    ALOGV("%d,%d,%d,%d,%d,%d", hdl, ultracmd, param1, param2, param3, param4);
    ALOGE("%s not support!!!",__FUNCTION__);
    return 0;
}

extern "C"
int PowerHal_Wrap_getGameModeStatus(void)
{
    int status = -1;

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->querySysInfo(MTKPOWER_CMD_GET_POWERHAL_MODE_STATUS, GAME_MODE, &status);
        }
    }
    pthread_mutex_unlock(&sMutex);

    return status;
}

extern "C"
int PowerHal_TouchBoost(int32_t enable)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->mtkPowerHint(MTKPOWER_HINT_APP_TOUCH, enable);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s",__FUNCTION__);
            Return<void> ret = gMtkPowerHal->mtkPowerHint(MTKPOWER_HINT_APP_TOUCH, enable);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);

    return 0;
}

extern "C"
int PowerHal_Wrap_setSysInfo(int32_t type, const char *data)
{
    int result = 0;

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->setSysInfo(type, data, &result);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s, type:%d",__FUNCTION__, type);
            Return<int32_t> ret = gMtkPowerHal->setSysInfo(type, data);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                result = ret;
                ALOGI("%s, result:%d", __FUNCTION__, result);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return result;
}

extern "C"
int PowerHal_Wrap_setSysInfoAsync(int32_t type, const char *data)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->setSysInfoAsync(type, data);
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGD("%s, type:%d",__FUNCTION__, type);
            Return<void> ret = gMtkPowerHal->setSysInfoAsync(type, data);
            processReturn(ret, __FUNCTION__);
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_EnableMultiDisplayMode(int32_t enable, int32_t fps)
{
    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ALOGI("%s, enable:%d fps:%d CurMultiDisplayMode:%d",__FUNCTION__, enable, fps, CurMultiDisplayMode);
            EnableMultiDisplayMode = enable;
            dfrc_fps = fps;
            if (CurMultiDisplayMode != -1) {
                ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(CurMultiDisplayMode, 0);
                CurMultiDisplayMode = -1;
            }
            if (EnableMultiDisplayMode) {
                if (dfrc_fps == 60) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60;
                    ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                } else if (dfrc_fps == 90) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90;
                    ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                } else if (dfrc_fps == 120) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120;
                    ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                }
            }
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            ALOGI("%s, enable:%d fps:%d CurMultiDisplayMode:%d",__FUNCTION__, enable, fps, CurMultiDisplayMode);
            EnableMultiDisplayMode = enable;
            dfrc_fps = fps;

            if (CurMultiDisplayMode != -1) {
                Return<void> ret = gMtkPowerHal->mtkCusPowerHint(CurMultiDisplayMode, 0);
                processReturn(ret, __FUNCTION__);
                CurMultiDisplayMode = -1;
            }

            if (EnableMultiDisplayMode) {
                if (dfrc_fps == 60) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60;
                    Return<void> ret = gMtkPowerHal->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                    processReturn(ret, __FUNCTION__);
                } else if (dfrc_fps == 90) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90;
                    Return<void> ret = gMtkPowerHal->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                    processReturn(ret, __FUNCTION__);
                } else if (dfrc_fps == 120) {
                    CurMultiDisplayMode = MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120;
                    Return<void> ret = gMtkPowerHal->mtkCusPowerHint(CurMultiDisplayMode, MTKPOWER_HINT_ALWAYS_ENABLE);
                    processReturn(ret, __FUNCTION__);
                }
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_EnableSFLowPowerMode(int32_t enable)
{
    pthread_mutex_lock(&sMutex);
    ALOGI("%s, enable:%d",__FUNCTION__, enable);

    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            if (enable) {
                ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(MTKPOWER_HINT_SF_LOW_POWER_MODE, MTKPOWER_HINT_ALWAYS_ENABLE);
            } else {
                ndk::ScopedAStatus ret = gIMtkPowerService->mtkCusPowerHint(MTKPOWER_HINT_SF_LOW_POWER_MODE, 0);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_EnableGameMode(int32_t enable, int32_t pid)
{
    pthread_mutex_lock(&sMutex);
    int result = 0, duration = 0, ret_hdl = 0, gameModeIndex = -1, gameModeEmptyIndex = -1, i = 0 , r = 0;
    int scnHint = MTKPOWER_HINT_WHITELIST_LAUNCH, scnDuration = 1000;
    char str[64] = "";
    std::vector<int32_t> rscList;

    for(i=(GAME_MAX_NUM-1); i>=0; i--) {
        if(gameModePidList[i] == pid) {
            gameModeIndex = i;
        }
        if(gameModePidList[i] == 0) {
            gameModeEmptyIndex = i;
        }
    }

    if(gameModeIndex == -1 && enable == 0) {
        ALOGE("%s, game mode pid %d has been disabled!", __FUNCTION__, pid);
        goto out;
    }

    if(gameModeIndex != -1 && enable == 1) {
        ALOGE("%s, game mode pid %d has been enabled!", __FUNCTION__, pid);
        goto out;
    }

    if(gameModeIndex == -1 && gameModeEmptyIndex == -1) {
        ALOGE("%s, gameModeIndex error", __FUNCTION__);
        goto out;
    }

    if(gameModeIndex == -1) {
        gameModeIndex = gameModeEmptyIndex;
    }

    gameModePidList[gameModeIndex] = pid;

    ALOGI("%s, enable:%d pid:%d", __FUNCTION__, enable, pid);
    ALOGD("%s, gameModeIndex:%d gameModeEmptyIndex:%d",__FUNCTION__, gameModeIndex, gameModeEmptyIndex);

    if (snprintf(str, 64, "%d", pid) < 0) {
        ALOGE("%s, snprintf error", __FUNCTION__);
        goto out;
    }

    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            ndk::ScopedAStatus ret = gIMtkPowerService->setSysInfo(SETSYS_GAME_MODE_PID, str, &result);
            if (enable == 1) {
                if(mtkpowerServiceVersion == 1) {
                    int rsc_temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, pid, PERF_RES_POWERHAL_GAME_MODE_ENABLE, enable};
                    rscList.assign(rsc_temp_list, (rsc_temp_list + 4));
                    ndk::ScopedAStatus ret = gIMtkPowerService->perfLockAcquire(gameModeHandle[gameModeIndex], duration, rscList, (int)getpid(), (int)gettid(), &ret_hdl);
                } else {
                    int rsc_temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, pid, PERF_RES_POWERHAL_GAME_MODE_ENABLE, enable, PERF_RES_POWERHAL_PRIORITY, MODE_PRIORITY};
                    rscList.assign(rsc_temp_list, (rsc_temp_list + 6));
                    ndk::ScopedAStatus ret = gIMtkPowerService->perfCusLockHintWithData(MTKPOWER_HINT_GAME_MODE, duration, (int)getpid(), rscList, &ret_hdl);
                }
                gameModeHandle[gameModeIndex] = ret_hdl;
                ALOGI("%s, [Enable Game Mode] hdl: %d, gameModeIndex: %d", __FUNCTION__, gameModeHandle[gameModeIndex], gameModeIndex);
                ret = gIMtkPowerService->perfCusLockHint(scnHint, scnDuration, (int)getpid(), &ret_hdl);
                gameScnHintHandle[gameModeIndex] = ret_hdl;
                ALOGI("%s, [Enable Game Mode SCN hint] hdl: %d, gameScnHintHandle: %d", __FUNCTION__, gameScnHintHandle[gameModeIndex], gameModeIndex);
            } else if (enable == 0) {
                // Disable all game modes so that the rest of game mode can be enabled again
                for(i=0; i<GAME_MAX_NUM; i++) {
                    if(gameModePidList[i] != 0) {
                        ndk::ScopedAStatus ret = gIMtkPowerService->perfLockReleaseSync(gameModeHandle[i], (int)getpid(), &r);
                        ALOGI("%s, [Disable Game Mode] hdl: %d", __FUNCTION__, gameModeHandle[i]);
                        gameModeHandle[i] = 0;
                        gameScnHintHandle[i] = 0;
                    }
                }
                gameModeHandle[gameModeIndex] = 0;
                gameScnHintHandle[gameModeIndex] = 0;
                gameModePidList[gameModeIndex] = 0;

                // If need, Enable the game mode of the rest of games
                for(i=0; i<GAME_MAX_NUM; i++) {
                    if(gameModePidList[i] != 0) {
                        if(mtkpowerServiceVersion == 1) {
                            int temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, gameModePidList[i], PERF_RES_POWERHAL_GAME_MODE_ENABLE, 1};
                            rscList.assign(temp_list, (temp_list + 4));
                            ndk::ScopedAStatus ret = gIMtkPowerService->perfLockAcquire(gameModeHandle[i], duration, rscList, (int)getpid(), (int)gettid(), &ret_hdl);
                        } else {
                            int temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, gameModePidList[i], PERF_RES_POWERHAL_GAME_MODE_ENABLE, 1, PERF_RES_POWERHAL_PRIORITY, MODE_PRIORITY};
                            rscList.assign(temp_list, (temp_list + 6));
                            ndk::ScopedAStatus ret = gIMtkPowerService->perfCusLockHintWithData(MTKPOWER_HINT_GAME_MODE, duration, (int)getpid(), rscList, &ret_hdl);
                            ALOGI("%s, gameCount > 1, [Enable Game Mode] hdl: %d, gameModeIndex: %d", __FUNCTION__, gameModeHandle[i], i);
                        }
                        gameModeHandle[i] = ret_hdl;
                    }
                }
            }
        }
    } else {
        if (getMtkPowerHal() && gMtkPowerHal != nullptr) {
            Return<int32_t> ret = gMtkPowerHal->setSysInfo(SETSYS_GAME_MODE_PID, str);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                result = ret;
                ALOGD("%s, result:%d", __FUNCTION__, result);
            }

            if (getMtkPerfHal() && gMtkPerfHal != nullptr) {
                if (enable == 1) {
                    int rsc_temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, pid, PERF_RES_POWERHAL_GAME_MODE_ENABLE, enable};
                    rscList.assign(rsc_temp_list, (rsc_temp_list + 4));
                    Return<int32_t> ret = gMtkPerfHal->perfLockAcquire(gameModeHandle[gameModeIndex], duration, rscList, (int)gettid());
                    if (processReturnWithInt32(ret, __FUNCTION__)) {
                        gameModeHandle[gameModeIndex] = ret;
                        ALOGI("[%s][Enable Game Mode] hdl: %d, gameModeIndex: %d", __FUNCTION__, gameModeHandle[gameModeIndex], gameModeIndex);
                    }
                    ret = gMtkPerfHal->perfCusLockHint(scnHint, scnDuration);
                    if (processReturnWithInt32(ret, __FUNCTION__)) {
                        gameScnHintHandle[gameModeIndex] = ret;
                        ALOGI("[%s][Enable Game Mode SCN hint] hdl: %d, gameScnHintHandle: %d", __FUNCTION__, gameScnHintHandle[gameModeIndex], gameModeIndex);
                    }
                } else if (enable == 0) {
                    for(i=0; i<GAME_MAX_NUM; i++) {
                        if(gameModePidList[i] != 0) {
                            Return<int32_t> ret = gMtkPerfHal->perfLockReleaseSync(gameModeHandle[i], (int)gettid());
                            if (processReturnWithInt32(ret, __FUNCTION__)) {
                                ALOGI("[%s][Disable Game Mode] hdl: %d", __FUNCTION__, gameModeHandle[i]);
                            }
                            gameModeHandle[i] = 0;
                            gameScnHintHandle[i] = 0;
                        }
                    }
                    gameModeHandle[gameModeIndex] = 0;
                    gameScnHintHandle[gameModeIndex] = 0;
                    gameModePidList[gameModeIndex] = 0;

                    for(i=0; i<GAME_MAX_NUM; i++) {
                        if(gameModePidList[i] != 0) {
                            int temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, gameModePidList[i], PERF_RES_POWERHAL_GAME_MODE_ENABLE, 1};
                            rscList.assign(temp_list, (temp_list + 4));
                            Return<int32_t> ret = gMtkPerfHal->perfLockAcquire(gameModeHandle[i], duration, rscList, (int)gettid());
                            ALOGI("[%s] gameCount > 1, [Enable Game Mode] hdl: %d, gameModeIndex: %d", __FUNCTION__, gameModeHandle[i], i);
                            gameModeHandle[i] = ret;
                        }
                    }
                }
            }
        }
    }

out:
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_EnableMode(int32_t cusHint, int32_t enable, int32_t pid, int *handle)
{
    pthread_mutex_lock(&sMutex);
    int result = 0, sysMode = -1;
    std::vector<int32_t> rscList;
    int rsc_temp_list[] = {PERF_RES_FPS_FPSGO_RENDER_PID, pid, PERF_RES_POWERHAL_PRIORITY, MODE_PRIORITY};
    char str[64] = "";

    rscList.assign(rsc_temp_list, rsc_temp_list + 4);

    if (snprintf(str, 64, "%d", pid) < 0) {
        ALOGE("%s, snprintf error", __FUNCTION__);
        goto out;
    }

    switch(cusHint) {
        case MTKPOWER_HINT_CAMERA_MODE:
            sysMode = SETSYS_CAMERA_MODE_PID;
            break;

        default:
            break;
    }

    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            if (enable == 1) {
                if(mtkpowerServiceVersion == 1) {
                    ndk::ScopedAStatus ret = gIMtkPowerService->setSysInfo(sysMode, str, &result);
                    ret = gIMtkPowerService->perfCusLockHint(cusHint, MTKPOWER_HINT_ALWAYS_ENABLE, (int)gettid(), &result);
                } else {
                    ndk::ScopedAStatus ret = gIMtkPowerService->perfCusLockHintWithData(cusHint, MTKPOWER_HINT_ALWAYS_ENABLE, (int)getpid(), rscList, &result);
                }
                *handle = result;
            } else if (enable == 0) {
                ndk::ScopedAStatus ret = gIMtkPowerService->perfLockReleaseSync(*handle, (int)getpid(), &result);
                *handle = 0;
            }
        }
    } else {
        if ((getMtkPowerHal() && gMtkPowerHal != nullptr) && (getMtkPerfHal() && gMtkPerfHal != nullptr)) {
            Return<int32_t> ret = gMtkPowerHal->setSysInfo(sysMode, str);
            if (processReturnWithInt32(ret, __FUNCTION__)) {
                result = ret;
                ALOGI("%s, result:%d", __FUNCTION__, result);
            }

            if (enable == 1) {
                Return<int32_t> ret = gMtkPerfHal->perfCusLockHint(cusHint, MTKPOWER_HINT_ALWAYS_ENABLE);
                *handle = ret;
                processReturnWithInt32(ret, __FUNCTION__);
            } else if (enable == 0) {
                Return<int32_t> ret = gMtkPerfHal->perfLockReleaseSync(*handle, 0);
                *handle = 0;
                processReturnWithInt32(ret, __FUNCTION__);
            }
        }
    }

out:
    pthread_mutex_unlock(&sMutex);
    return 0;
}

extern "C"
int PowerHal_Wrap_EnableCameraMode(int32_t enable, int32_t pid)
{
    if(camera_mode_on == enable) {
        ALOGI("%s, camera mode has been enabled or not enabled: %d", __FUNCTION__, enable);
        return 0;
    }

    camera_mode_on = enable;

    ALOGI("%s, camera enable:%d pid:%d",__FUNCTION__, enable, pid);
    if(!PowerHal_Wrap_EnableMode(MTKPOWER_HINT_CAMERA_MODE, enable, pid, &camera_mode_handle)) {
        return -1;
    }

    return 0;
}



extern "C"
int RESM_setThreadHintPolicy(int pid, const char *thread_name, int tid,
    int mode, int matching_num, int prio, int cpu_mask, int set_exclusive,
    int loading_ub, int loading_lb, int bhr, int limit_min_freq, int limit_max_freq,
    int set_rescue, int rescue_f_opp, int rescue_c_freq, int rescue_time)
{
    int ret = -1;

    ALOGI("[%s] pid=%d, thread_name=%s, tid=%d, "
        "mode=%d, matching_num=%d, prio=%d, cpu_mask=%d, set_exclusive=%d, "
        "loading_ub=%d, loading_lb=%d, bhr=%d, limit_min_freq=%d, limit_max_freq=%d, "
        "set_rescue=%d, rescue_f_opp=%d, rescue_c_freq=%d, rescue_time=%d",
        __FUNCTION__, pid, thread_name, tid,
        mode, matching_num, prio, cpu_mask, set_exclusive,
        loading_ub, loading_lb, bhr, limit_min_freq, limit_max_freq,
        set_rescue, rescue_f_opp, rescue_c_freq, rescue_time);

    pthread_mutex_lock(&sMutex);
    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            if (mtkpowerServiceVersion >= 3) {
                ThreadHintParams params;

                params.pid = pid;
                if (thread_name != nullptr)
                    params.thread_name = thread_name;
                params.tid = tid;
                params.mode = mode;
                params.matching_num = matching_num;
                params.prio = prio;
                params.cpu_mask = cpu_mask;
                params.set_exclusive = set_exclusive;
                params.loading_ub = loading_ub;
                params.loading_lb = loading_lb;
                params.limit_min_freq = limit_min_freq;
                params.limit_max_freq = limit_max_freq;
                params.bhr = bhr;
                params.set_rescue = set_rescue;
                params.rescue_f_opp = rescue_f_opp;
                params.rescue_c_freq = rescue_c_freq;
                params.rescue_time = rescue_time;

                auto r = gIMtkPowerService->setThreadHintPolicy(params, &ret);
                if (!r.isOk()) {
                    ALOGE("[%s] Failed to setThreadHintPolicy", __FUNCTION__);
                }

            } else {
                ALOGE("[%s] Not support! Device not launching with mtkpower-V3 and beyond. (mtkpowerServiceVersion=%d)",
                    __FUNCTION__, mtkpowerServiceVersion);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);

    ALOGD("[%s] ret=%d", __FUNCTION__, ret);

    return ret;
}

extern "C"
int RESM_enableThreadHint(int tgid, bool enable)
{
    int ret = -1;

    ALOGI("[%s] tgid=%d, enable=%d", __FUNCTION__, tgid, enable);

    pthread_mutex_lock(&sMutex);

    // To avoid conflicts between RESM API and the auto thread hint controller,
    // Disable the controller when the API is in use.
    // When the API is not in use, re-enable the controller.
    if (enable)
        applistSetThreadHintControllerEnabled(false);
    else
        applistSetThreadHintControllerEnabled(true);


    if (check_IMtkPowerService_supported() == 1) {
        if (getIMtkPowerService() && gIMtkPowerService != nullptr) {
            if (mtkpowerServiceVersion >= 3) {
                auto r = gIMtkPowerService->enableThreadHint(tgid, enable, &ret);
                if (!r.isOk()) {
                    ALOGE("[%s] Failed to enableThreadHint", __FUNCTION__);
                }
            } else {
                ALOGE("[%s] Not support! Device not launching with PowerHAL V3 and beyond. (mtkpowerServiceVersion=%d)",
                    __FUNCTION__, mtkpowerServiceVersion);
            }
        }
    }
    pthread_mutex_unlock(&sMutex);

    ALOGD("[%s] ret=%d", __FUNCTION__, ret);

    return ret;
}



//} // namespace


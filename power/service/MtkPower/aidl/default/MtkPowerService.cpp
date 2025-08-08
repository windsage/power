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

#include <log/log.h>
#include <cutils/trace.h>
#include <string.h>
#include <hardware/hardware.h>
#include <hardware/power.h>
#include <time.h>
#include "MtkPowerService.h"
#include "mtkpower_hint.h"
#include "mtkpower_types.h"
#include "libpowerhal_wrap.h"
#include "mtkperf_resource.h"
#include "utility_fps.h"



namespace aidl {
namespace vendor {
namespace mediatek {
namespace hardware {
namespace mtkpower {

using ::ndk::ScopedAStatus;
using ::aidl::vendor::mediatek::hardware::mtkpower::ThreadHintParams;

#define LOG_E(fmt, arg...) ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...) ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...) ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...) ALOGV("[%s] " fmt, __func__, ##arg)
#define NAME_SIZE_MAX 256
#define STR_LEN_MAX 256
#define LOG_TAG "libPowerHal"
#define SYSFS_GAME_LOOM_TASK_CFG_PATH "/sys/kernel/game/loom_task_cfg"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

static int gtCusHintTbl[MTKPOWER_HINT_NUM];
static char currPackname[NAME_SIZE_MAX];
static char currActname[NAME_SIZE_MAX];
static int bDuringProcessCreate = 0;

MtkPowerService::MtkPowerService() {
    int i;
    for (i = 0; i < MTKPOWER_HINT_NUM; i++) {
        gtCusHintTbl[i] = 0;
    }
    libpowerhal_wrap_Init(1);
}

MtkPowerService::~MtkPowerService() {}

ndk::ScopedAStatus MtkPowerService::perfLockAcquire(int hdl, int duration, const std::vector<int>& boostList, int pid, int reserved, int* _aidl_return) {
    LOG_D("hdl:%d, duration:%d, pid:%d, reserved:%d", hdl, duration, pid, reserved);

    int size;

    size = boostList.size();
    LOG_D("data size:%d", size);
    if (size % 2 != 0 || size > MAX_ARGS_PER_REQUEST || size == 0) {
        LOG_E("wrong data size:%d", size);
        *_aidl_return = 0;
    }

    *_aidl_return = libpowerhal_wrap_LockAcq((int*)boostList.data(), hdl, size, pid, reserved, duration);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::perfCusLockHint(int hint, int duration, int pid, int* _aidl_return) {
    LOG_D("hint:%d, duration:%d, pid:%d", hint, duration, pid);

    *_aidl_return = libpowerhal_wrap_CusLockHint(hint, duration, pid);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::perfCusLockHintWithData(int hint, int duration, int pid, const std::vector<int>& extendedList, int* _aidl_return) {
    LOG_D("hint:%d, duration:%d, pid:%d", hint, duration, pid);

    *_aidl_return = libpowerhal_wrap_CusLockHint_data(hint, duration, pid, (int*)extendedList.data(), extendedList.size());

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::perfLockRelease(int hdl, int reserved) {
    LOG_D("hdl:%d pid:%d", hdl, reserved);
    int scn_pid = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_SCN_PID, hdl);
    if (reserved == scn_pid)
        libpowerhal_wrap_LockRel(hdl);
    else
        LOG_E("PID %d is error, %d", reserved, scn_pid);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::perfLockReleaseSync(int hdl, int reserved, int* _aidl_return) {
    LOG_D("hdl:%d pid:%d", hdl, reserved);
    int scn_pid = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_SCN_PID, hdl);
    if (reserved == scn_pid)
        *_aidl_return = libpowerhal_wrap_LockRel(hdl);
    else {
        *_aidl_return = -1;
        LOG_E("PID %d is error, %d", reserved, scn_pid);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::mtkPowerHint(int hint, int data)  {
    LOG_D("hint:%d, data:%d", hint, data);

    int hdl1 = 0, hdl2 = 0, hold_t = 0;
    int ext_hint = 0, ext_hold_t = 0, ext_hdl = 0;
    const int pid = (int)getpid();

    if (hint < MTKPOWER_HINT_BASE || hint >= MTKPOWER_HINT_NUM) {
        LOG_E("unsupport hint:%d", hint);
        return ndk::ScopedAStatus::ok();
    }

    hdl1 = gtCusHintTbl[hint];
    if (data > 0) {
        /* handle specific hint */
        switch(hint) {
        case MTKPOWER_HINT_PROCESS_CREATE:
            if(data > 1)
                bDuringProcessCreate = 1; // for white list boost
            break;

        case MTKPOWER_HINT_APP_TOUCH:
            data = 10000; // 10sec. timeout
            break;

        case MTKPOWER_HINT_PMS_INSTALL:
            if (libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_INSTALL_MAX_DURATION, 0) > 0)
                data = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_INSTALL_MAX_DURATION, 0);
            break;

        default:
            break;
        }

        /* acquire perf lock */
        hdl2 = libpowerhal_wrap_CusLockHint(hint, data, pid);
        gtCusHintTbl[hint] = hdl2;
        if (hdl1 != 0) {
            libpowerhal_wrap_LockRel(hdl1);
        }

    } else {
        /* handle specific hint */
        switch(hint) {
        case MTKPOWER_HINT_PROCESS_CREATE:
            bDuringProcessCreate = 0; // for white list boost
            break;

        default:
            break;
        }

        /* release lock */
        if (hdl1 != 0) {
            hold_t = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_POWER_HDL_HOLD_TIME, hdl1); // additional hold time
            ext_hint = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT, hdl1);
            ext_hold_t = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT_HOLD_TIME, hdl1);

            if (hold_t > 0 || ext_hold_t > 0)
                LOG_I("hint:%d, hold:%d, ext:%d, ext_hold:%d", hint, hold_t, ext_hint, ext_hold_t);
            if (hold_t > 0) {
                hdl2 = libpowerhal_wrap_CusLockHint(hint, hold_t, pid);
            }
            libpowerhal_wrap_LockRel(hdl1);

            if (ext_hint > 0 && ext_hold_t > 0) {
                ext_hdl = libpowerhal_wrap_CusLockHint(ext_hint, ext_hold_t, pid);
                gtCusHintTbl[ext_hint] = ext_hdl;
            }
        }
        gtCusHintTbl[hint] = hdl2;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::mtkCusPowerHint(int hint, int data)  {
    LOG_D("hint:%d, data:%d", hint, data);

    int hdl1, hdl2;
    const int pid = (int)getpid();;

    if(hint < MTKPOWER_HINT_BASE || hint >= MTKPOWER_HINT_NUM) {
        LOG_E("unsupport hint:%d", hint);
        return ndk::ScopedAStatus::ok();
    }

    hdl1 = gtCusHintTbl[hint];
    if (data > 0) {
        hdl2 = libpowerhal_wrap_CusLockHint(hint, data, pid);
        gtCusHintTbl[hint] = hdl2;
        if (hdl1 != 0) {
            libpowerhal_wrap_LockRel(hdl1);
        }
    } else {
        if (hdl1 != 0) {
            libpowerhal_wrap_LockRel(hdl1);
        }
        gtCusHintTbl[hint] = 0;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::querySysInfo(int cmd, int param, int* _aidl_return)  {
    LOG_D("querySysInfo cmd:%d, param:%d", (int)cmd, param);
    *_aidl_return = libpowerhal_wrap_UserGetCapability(cmd, param);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::setSysInfo(int type, const std::string& data, int* _aidl_return) {
    *_aidl_return = libpowerhal_wrap_SetSysInfo(type, data.c_str());

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::setSysInfoAsync(int type, const std::string& data) {
    libpowerhal_wrap_SetSysInfo(type, data.c_str());

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::setMtkPowerCallback(const std::shared_ptr<::aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback, int* _aidl_return) {
    if (callback != nullptr) {
        LOG_D("setMtkPowerCallback not support !!!");
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::setMtkScnUpdateCallback(int scn, const std::shared_ptr<::aidl::vendor::mediatek::hardware::mtkpower::IMtkPowerCallback>& callback, int* _aidl_return) {
    if (callback != nullptr) {
        libpowerhal_wrap_SetScnUpdateCallback(scn, callback);
        LOG_D("[%s]setMtkScnUpdateCallback set scn %x callback", __FUNCTION__, scn);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::setThreadHintPolicy(const ThreadHintParams& params, int* _aidl_return)
{
    int ret = -1;

    LOG_I("pid=%d, thread_name=%s, tid=%d, "
        "mode=%d, matching_num=%d, prio=%d, cpu_mask=%d, set_exclusive=%d, "
        "loading_ub=%d, loading_lb=%d, bhr=%d, limit_min_freq=%d, limit_max_freq=%d, "
        "set_rescue=%d, rescue_f_opp=%d, rescue_c_freq=%d, rescue_time=%d",
        params.pid, params.thread_name.c_str(), params.tid,
        params.mode, params.matching_num, params.prio, params.cpu_mask, params.set_exclusive,
        params.loading_ub, params.loading_lb, params.bhr, params.limit_min_freq, params.limit_max_freq,
        params.set_rescue, params.rescue_f_opp, params.rescue_c_freq, params.rescue_time);

    char buf[1024];
    buf[0] = '\0';

    int s = snprintf(buf, sizeof(buf),
        "%d %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
        params.pid,
        params.thread_name.c_str(),
        params.tid,
        params.mode,
        params.matching_num,
        params.prio,
        params.cpu_mask,
        params.set_exclusive,
        params.loading_ub,
        params.loading_lb,
        params.limit_min_freq,
        params.limit_max_freq,
        params.bhr,
        params.set_rescue,
        params.rescue_f_opp,
        params.rescue_c_freq,
        params.rescue_time
        );

    if (s < 0) {
        LOG_E("snprintf error");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    const char* path = SYSFS_GAME_LOOM_TASK_CFG_PATH;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        LOG_E("cannot open %s", path);
        char *err_str = strerror(errno);
        LOG_E("%s (errno=%d)", err_str, errno);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    ssize_t len = strlen(buf);
    ret = write(fd, buf, len);
    close(fd);

    if (ret == -1) {
        LOG_E("cannot write %s", path);
        char *err_str = strerror(errno);
        LOG_E("%s (errno=%d)", err_str, errno);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    LOG_I("%s > %s", buf, SYSFS_GAME_LOOM_TASK_CFG_PATH);

    _aidl_return = &ret;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MtkPowerService::enableThreadHint(int tgid, bool enable, int* _aidl_return)
{
    int ret = -1;
    char buf[1024];
    buf[0] = '\0';

    LOG_I("tgid=%d, enable=%d", tgid, enable);

    if (snprintf(buf, sizeof(buf), "%d %d", tgid, enable) < 0) {
        LOG_E("snprintf error");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    int fd = open(PATH_LOOM_ENABLE_BY_PID, O_WRONLY);

    if (fd < 0) {
        LOG_E("cannot open %s", PATH_LOOM_ENABLE_BY_PID);
        char *err_str = strerror(errno);
        LOG_E("%s (errno=%d)", err_str, errno);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    ssize_t len = strlen(buf);
    ret = write(fd, buf, len);
    close(fd);

    if (ret == -1) {
        LOG_E("cannot write %s", PATH_LOOM_ENABLE_BY_PID);
        char *err_str = strerror(errno);
        LOG_E("%s (errno=%d)", err_str, errno);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    LOG_I("%s > %s", buf, PATH_LOOM_ENABLE_BY_PID);

    _aidl_return = &ret;

    return ndk::ScopedAStatus::ok();
}





}  // namespace mtkpower
}  // namespace hardware
}  // namespace mediatek
}  // namespace vendor
}  // namespace aidl
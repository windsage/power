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

#define LOG_TAG "mtkpower@impl"

#include "Power.h"
#include "PowerHintSession.h"
#include "PowerHintSessionManager.h"
#include <log/log.h>
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <inttypes.h>
#include <thread>
#include <mtkpower_types.h>
#include <set>
#include <pthread.h>
#include "mtkperf_resource.h"
#include "libpowerhal_wrap.h"
#include "mtkpower_hint.h"
#include "include/sysInfo.cpp"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

int modeCount = 0;
static long devicePreferredRate = -1;
static float globalCpuHeadroom = 0.0f;
static float globalGpuHeadroom = 0.0f;

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace mediatek {

using android::hardware::power::Boost;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::aidl::android::hardware::power::ChannelMessage;
using ::android::AidlMessageQueue;
using namespace std::chrono_literals;

#define LAUNCH_HINT_MAX_TIME    30000 // 30000 ms
#define INTERACTION_DEFAULT_T   80 // 80 ms
#define LOADING_BOOST_MAX_DURATION  5 * 1000 // 5 seconds
#define MAX_MODE_COUNT     100 // Avoid the modeCount overflow

const std::vector<Boost> kBoosts{ndk::enum_range<Boost>().begin(), ndk::enum_range<Boost>().end()};
const std::vector<Mode> kModes{ndk::enum_range<Mode>().begin(), ndk::enum_range<Mode>().end()};

const std::set<SessionTag> unsupportedTags = {
    SessionTag::SYSUI,
    SessionTag::HWUI,
    SessionTag::SURFACEFLINGER,
};

static int bBacklightOn = 1;
static int gtCusHintTbl[MTKPOWER_HINT_NUM];

Power::Power() {
    LOG_I("MTKPOWER_HINT_NUM:%d", MTKPOWER_HINT_NUM);
    for (int i=0; i < MTKPOWER_HINT_NUM; i++) {
        gtCusHintTbl[i] = 0;
    }
    libpowerhal_wrap_Init(1);

    // ADPF
    _ADPF_PACKAGE data;
    data.cmd = ADPF::GET_HINT_SESSION_PREFERED_RATE;
    devicePreferredRate = get_max_refresh_rate();
    LOG_I("devicePreferredRate: %ld", devicePreferredRate);

    if(devicePreferredRate == -1) {
        LOG_E("Get devicePreferredRate error :%d", devicePreferredRate);
    }
}

Power::~Power() {}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {

    int dur, hdl1, hdl2, hint, trigger_lock = 0, ret;
    LOG_I("type:%d, enabled:%d", type, enabled);

    switch(type) {

    case Mode::LAUNCH:
        trigger_lock = 1;
        hint = LAUNCH; // NOTICE: This "LAUNCH" number is defined in mtkpower_hint.h not Mode.aidl

        if(enabled > 0 && bBacklightOn) {
            dur = LAUNCH_HINT_MAX_TIME; // ALPS04995560
        } else {
            dur = -1; // -1 means disable
        }
        break;

    case Mode::GAME_LOADING:
        trigger_lock = 1;
        hint = GAME_LOADING;
        LOG_I("Call the game modeCount: %d", modeCount);

        if (enabled > 0) {
            dur = LOADING_BOOST_MAX_DURATION;
            modeCount++;
            if(modeCount == MAX_MODE_COUNT) {
                modeCount = 0;
            }
        } else {
            dur = -1;
        }
        break;

    case Mode::INTERACTIVE:
        ret = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_INTERACTIVE_CONTROL, 0);
        if(ret) {
            if(enabled) {
                LOG_I("Restore All");
                libpowerhal_wrap_UserScnRestoreAll();
                bBacklightOn = 1;

            } else {
                libpowerhal_wrap_UserScnDisableAll();
                bBacklightOn = 0;
                LOG_I("Disable All");
            }
        }
        break;

    case Mode::DISPLAY_INACTIVE:
        LOG_I("Mode::DISPLAY_INACTIVE: %d", enabled);
        break;

    case Mode::FIXED_PERFORMANCE:
        LOG_I("Mode::FIXED_PERFORMANCE: %d", enabled);
        break;

    case Mode::SUSTAINED_PERFORMANCE:
        trigger_lock = 1;
        hint = SUSTAINED_PERFORMANCE; // mtkpower hint ID in mtkpower_hint.h
        if(enabled > 0) {
            dur = 0;  // 0 means infinite duration
        } else {
            dur = -1; // -1 means disable
        }
        LOG_I("Mode::SUSTAINED_PERFORMANCE: %d, %d", enabled, dur);
        break;

    default:
        LOG_E("unknown type");
        break;
    }

    /* need lock operation */
    if (trigger_lock) {
        hdl1 = gtCusHintTbl[hint];
        if (dur >= 0) {
            hdl2 = libpowerhal_wrap_CusLockHint(hint, dur, (int)getpid());
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
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    if (type >= kModes.front() && type <= kModes.back()) {
        LOG_I("supported:%d", type);
        *_aidl_return = true;
    } else {
        LOG_I("unsupported:%d", type);
        *_aidl_return = false;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {

    int hint, hdl;
    LOG_D("type:%d, durationMs:%d", type, durationMs);

    switch(type) {

    case Boost::INTERACTION:
        LOG_D("Boost::INTERACTION: %d", durationMs);
        hint = INTERACTION;
        durationMs = (durationMs > 0) ? durationMs : INTERACTION_DEFAULT_T;
        hdl = libpowerhal_wrap_CusLockHint(hint, durationMs, (int)getpid());
        break;

    case Boost::DISPLAY_UPDATE_IMMINENT:
        LOG_D("Boost::DISPLAY_UPDATE_IMMINENT: %d", durationMs);
        hint = DISPLAY_UPDATE_IMMINENT;
        hdl = libpowerhal_wrap_CusLockHint(hint, durationMs, (int)getpid());
        break;

    default:
        break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    if (type >= kBoosts.front() && type <= kBoosts.back()) {
        LOG_I("supported:%d", type);
        *_aidl_return = true;
    } else {
        LOG_I("unsupported:%d", type);
        *_aidl_return = false;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t* outNanoseconds) {
    if(devicePreferredRate == -1) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    *outNanoseconds = 1000000000/devicePreferredRate;
    LOG_I("ADPF: outNanoseconds = %ld", *outNanoseconds);

    return ndk::ScopedAStatus::ok();
}


ndk::ScopedAStatus Power::createHintSession(int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos, std::shared_ptr<IPowerHintSession>* _aidl_return) {
    LOG_I("ADPF: TGID=%d, UID=%d, targetDurationNanos=%ld", tgid, uid, durationNanos);

    SessionConfig config;
    return createHintSessionWithConfig(tgid, uid, threadIds, durationNanos, SessionTag::OTHER, &config, _aidl_return);
}

ndk::ScopedAStatus Power::createHintSessionWithConfig(
        int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos,
        SessionTag tag, SessionConfig* config, std::shared_ptr<IPowerHintSession>* _aidl_return) {

    LOG_I("ADPF: TGID=%d, UID=%d, targetDurationNanos=%ld, tag=%d", tgid, uid, durationNanos, tag);

    if (unsupportedTags.find(tag) != unsupportedTags.end()) {
        LOG_E("ADPF: Not supported for tag: %d", tag);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else
        LOG_I("ADPF: Creating hint session for tag: %d", tag);

    int sessionId = -1;

    if (threadIds.size() <= 0) {
        LOG_E("Error: threadIds.size() shouldn't be %d", threadIds.size());
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    } else {
        for (int i = 0; i < threadIds.size(); i ++)
            LOG_I("ADPF: threadIds[%d] = %d", i, threadIds[i]);
    }

    if (threadIds.size() >= ADPF_MAX_THREAD) {
        LOG_E("Error: threadIds.size %d exceeds the max size %d", threadIds.size(), ADPF_MAX_THREAD);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    sessionId = PowerHintSessionManager::getInstance()->checkPowerSesssion(tgid, uid, threadIds);

    if (sessionId == -1) {
        *_aidl_return = NULL;
        return ndk::ScopedAStatus::fromServiceSpecificError(-EINVAL);
    }

    std::shared_ptr<IPowerHintSession> powerHintSession = ndk::SharedRefBase::make<PowerHintSession>(sessionId, tgid, uid, threadIds, durationNanos, tag);
    *_aidl_return = powerHintSession;
    static_cast<PowerHintSession *>(_aidl_return->get())->getSessionConfig(config);

    int32_t threadIds_arr[ADPF_MAX_THREAD] = {0};
    std::copy(threadIds.begin(), threadIds.end(), threadIds_arr);

    _ADPF_PACKAGE msg;

    msg.cmd = ADPF::CREATE_HINT_SESSION;
    msg.sid = sessionId;
    msg.tgid = tgid;
    msg.uid = uid;
    msg.threadIds = threadIds_arr;
    msg.threadIds_size = threadIds.size();
    msg.durationNanos = durationNanos;
    adpf_sys_set_data(&msg);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getSessionChannel(int32_t tgid, int32_t uid, ChannelConfig* _aidl_return) {
    LOG_I("ADPF: tgid=%d, uid=%d", tgid, uid);

    static AidlMessageQueue<ChannelMessage, SynchronizedReadWrite> stubQueue{20, true};

    static std::thread stubThread([&] {
        ChannelMessage data;
        // This loop will only run while there is data waiting to be processed,
        // and blocks on a futex all other times
        while (stubQueue.readBlocking(&data, 1, 0)) {
            // handle message ...
        }
    });

    _aidl_return->channelDescriptor = stubQueue.dupeDesc();
    _aidl_return->readFlagBitmask = 0x01;
    _aidl_return->writeFlagBitmask = 0x02;
    _aidl_return->eventFlagDescriptor = std::nullopt;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::closeSessionChannel(int32_t tgid, int32_t uid) {
    LOG_I("ADPF: tgid=%d, uid=%d", tgid, uid);

    return ndk::ScopedAStatus::ok();
}


binder_status_t Power::dump(int fd, const char **, uint32_t) {
    std::string buf(::android::base::StringPrintf("GAME_LOADING\t%d\t\n", modeCount));

    LOG_I("dump mode: %d", modeCount);

    if (!::android::base::WriteStringToFd(buf, fd)) {
        PLOG(ERROR) << "Failed to dump state to fd";
    }
    fsync(fd);

    return STATUS_OK;
}

template <class T>
constexpr size_t enum_size() {
    return static_cast<size_t>(*(ndk::enum_range<T>().end() - 1)) + 1;
}

template <class E>
int64_t bitsForEnum() {
    return static_cast<int64_t>(std::bitset<enum_size<E>()>().set().to_ullong());
}

ndk::ScopedAStatus Power::getSupportInfo(SupportInfo* _aidl_return) {
    static SupportInfo supportInfo = {
        .usesSessions = true,
        .modes = bitsForEnum<Mode>(),
        .boosts = bitsForEnum<Boost>(),
        .sessionHints = bitsForEnum<SessionHint>(),
        .sessionModes = bitsForEnum<SessionMode>(),
        .sessionTags = bitsForEnum<SessionTag>(),

        .compositionData = {
            .isSupported = false,
            .disableGpuFences = false,
            .maxBatchSize = 1,
            .alwaysBatch = false,
        },

        .headroom = {
            .isCpuSupported = true,
            .isGpuSupported = true,
            // to avoid apps calling too frequently to perform side channel attack (such as interpreting user input)
            .cpuMinIntervalMillis = 500,
            .gpuMinIntervalMillis = 500,
        }
    };

    *_aidl_return = supportInfo;

    LOG_I("modes=%x", supportInfo.modes);
    LOG_I("boosts=%x", supportInfo.boosts);
    LOG_I("usesSessions=%d", supportInfo.usesSessions);
    LOG_I("sessionHints=%x", supportInfo.sessionHints);
    LOG_I("sessionModes=%x", supportInfo.sessionModes);
    LOG_I("sessionTags=%x", supportInfo.sessionTags);
    LOG_I("[headroom] isCpuSupported=%d, isGpuSupported=%d, cpuMinIntervalMillis=%d, gpuMinIntervalMillis=%d",
        supportInfo.headroom.isCpuSupported, supportInfo.headroom.isGpuSupported,
        supportInfo.headroom.cpuMinIntervalMillis, supportInfo.headroom.gpuMinIntervalMillis);
    LOG_I("[compositionData] isSupported=%d, disableGpuFences=%d, maxBatchSize=%d, alwaysBatch=%d",
        supportInfo.compositionData.isSupported, supportInfo.compositionData.disableGpuFences,
        supportInfo.compositionData.maxBatchSize, supportInfo.compositionData.alwaysBatch);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::sendCompositionData(const std::vector<CompositionData>& in_data) {
    ALOGI("[ADPF_sendCompositionData] ");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::sendCompositionUpdate(const CompositionUpdate& in_update) {
    ALOGI("[ADPF_sendCompositionUpdate] ");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getCpuHeadroom(const CpuHeadroomParams& params, CpuHeadroomResult* _aidl_return) {
    int headroom = get_cpu_headroom();
    if (headroom < 0 || headroom > 100)
        globalCpuHeadroom = std::numeric_limits<float>::quiet_NaN();
    else
        globalCpuHeadroom = static_cast<float>(headroom);

    LOG_D("ADPF: globalCpuHeadroom = %f", globalCpuHeadroom);
    _aidl_return->set<CpuHeadroomResult::globalHeadroom>(globalCpuHeadroom);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getGpuHeadroom(const GpuHeadroomParams& params, GpuHeadroomResult* _aidl_return) {
    int headroom = 100 - get_gpu_perf_index();
    if (headroom < 0 || headroom > 100)
        globalGpuHeadroom = std::numeric_limits<float>::quiet_NaN();
    else
        globalGpuHeadroom = static_cast<float>(headroom);

    LOG_D("ADPF: globalGpuHeadroom = %f", globalGpuHeadroom);
    _aidl_return->set<GpuHeadroomResult::globalHeadroom>(globalGpuHeadroom);

    return ndk::ScopedAStatus::ok();
}



}  // namespace mediatek
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl

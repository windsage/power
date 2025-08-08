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

#define LOG_TAG "mtkpower@impl"

#include "Power.h"
#include "PowerHintSession.h"
#include "PowerHintSessionManager.h"
#include "include/sysctl.h"
#include <inttypes.h>
#include <android-base/logging.h>
#include "libpowerhal_wrap.h"
#include "mtkpower_types.h"
#include "mtkperf_resource.h"


namespace aidl::android::hardware::power::impl::mediatek {

using ndk::ScopedAStatus;

int blc_boost_hdl = 0;

PowerHintSession::PowerHintSession(int32_t sessionId, int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos, SessionTag tag) {
    this->id = sessionId;
    this->tgid = tgid;
    this->uid = uid;
    this->threadIds = threadIds;
    this->durationNanos = durationNanos;
    this->tag = tag;

    int ret = PowerHintSessionManager::getInstance()->addPowerSession(this);
}

PowerHintSession::~PowerHintSession() {
    int ret = PowerHintSessionManager::getInstance()->removePowerSession(this);

    if(ret) {
        _ADPF_PACKAGE msg;
        msg.cmd = ADPF::CLOSE;
        msg.sid = this->id;
        adpf_sys_set_data(&msg);
    }else{
        LOG_D("session id %d have been closed!", this->id);
    }
}

ScopedAStatus PowerHintSession::updateTargetWorkDuration(int64_t targetDurationNanos) {
    LOG_I("ADPF: tgid=%d, sid=%d, targetDurationNanos=%ld", this->tgid, this->id, targetDurationNanos);
    _ADPF_PACKAGE msg;

    msg.cmd = ADPF::UPDATE_TARGET_WORK_DURATION;
    msg.sid = this->id;
    msg.targetDurationNanos = targetDurationNanos;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::reportActualWorkDuration(const std::vector<WorkDuration>& durations) {
    for (int i = 0; i < durations.size(); i ++) {
        LOG_D("ADPF: tgid=%d, sid=%d, ts=%ld, startTs=%ld, presentTs=%ld, dur=%ld, cpuDur=%ld, gpuDur=%ld",
            this->tgid,
            this->id,
            durations[i].timeStampNanos,
            durations[i].workPeriodStartTimestampNanos,
            durations[i].intendedPresentTimestampNanos,
            durations[i].durationNanos,
            durations[i].cpuDurationNanos,
            durations[i].gpuDurationNanos
        );
    }

    if(durations.size() >= ADPF_MAX_THREAD) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    _ADPF_PACKAGE msg;
    struct _WORK_DURATION work_duration_arr[ADPF_MAX_THREAD] = {0};

    msg.cmd = ADPF::REPORT_ACTUAL_WORK_DURATION;
    msg.sid = this->id;

    for(int i=0; i<durations.size(); i++) {
        work_duration_arr[i].timeStampNanos = durations[i].timeStampNanos;
        work_duration_arr[i].durationNanos = durations[i].durationNanos;
    }

    msg.workDuration = work_duration_arr;
    msg.work_duration_size = durations.size();
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::pause() {
    _ADPF_PACKAGE msg;

    LOG_I("ADPF: tgid=%d, sid=%d", this->tgid, this->id);

    msg.cmd = ADPF::PAUSE;
    msg.sid = this->id;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::resume() {
    _ADPF_PACKAGE msg;

    LOG_I("ADPF: tgid=%d, sid=%d", this->tgid, this->id);

    msg.cmd = ADPF::RESUME;
    msg.sid = this->id;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::close() {
    _ADPF_PACKAGE msg;

    LOG_I("ADPF: tgid=%d, sid=%d", this->tgid, this->id);

    int ret = PowerHintSessionManager::getInstance()->removePowerSession(this);
    if(ret) {
        msg.cmd = ADPF::CLOSE;
        msg.sid = this->id;
        adpf_sys_set_data(&msg);
    }else{
        LOG_E("session id %d does not exit!", this->id);
    }

    return ScopedAStatus::ok();
}

int predictWorkload(int pid, int blc_boost_scale, int blc_boost_duration)
{
    int perf_lock_rsc[] = {PERF_RES_FPS_FBT_BLC_BOOST, blc_boost_scale};
    int size = sizeof(perf_lock_rsc)/sizeof(int);
    blc_boost_hdl = libpowerhal_wrap_LockAcq(perf_lock_rsc, blc_boost_hdl, size, pid, pid, blc_boost_duration);

    return 0;
}

ScopedAStatus PowerHintSession::sendHint(SessionHint hint) {
    LOG_I("ADPF: tgid=%d, sid=%d, hint=%d", this->tgid, this->id, hint);
    int cpuLoadUpBoostScale = 0;
    int cpuLoadUpBoostDuration = 0;
    int cpuLoadSpikeBoostScale = 0;
    int cpuLoadSpikeBoostDuration = 0;
    int frameTimeMs = this->durationNanos / 1000000;

    _ADPF_PACKAGE msg;

    msg.cmd = ADPF::SENT_HINT;
    msg.sid = this->id;
    msg.hint = static_cast<int>(hint);
    adpf_sys_set_data(&msg);

    switch (hint)
    {
        case SessionHint::CPU_LOAD_UP:
            LOG_I("CPU_LOAD_UP");
            cpuLoadUpBoostScale = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_ADPF_CPU_LOAD_UP_BOOST_SCALE, 0);
            cpuLoadUpBoostDuration = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_ADPF_CPU_LOAD_UP_BOOST_DURATION, 0);
            if (cpuLoadUpBoostDuration == -1)
                cpuLoadUpBoostDuration = frameTimeMs;
            predictWorkload(this->tgid, cpuLoadUpBoostScale, cpuLoadUpBoostDuration);
            break;
        case SessionHint::CPU_LOAD_DOWN:
            LOG_I("CPU_LOAD_DOWN");
            break;
        case SessionHint::CPU_LOAD_RESET:
            LOG_I("CPU_LOAD_RESET");
            break;
        case SessionHint::CPU_LOAD_RESUME:
            LOG_I("CPU_LOAD_RESUME");
            break;
        case SessionHint::GPU_LOAD_UP:
            LOG_I("GPU_LOAD_UP");
            break;
        case SessionHint::GPU_LOAD_DOWN:
            LOG_I("GPU_LOAD_DOWN");
            break;
        case SessionHint::GPU_LOAD_RESET:
            LOG_I("GPU_LOAD_RESET");
            break;
        case SessionHint::POWER_EFFICIENCY:
            LOG_I("POWER_EFFICIENCY");
            break;
        case SessionHint::CPU_LOAD_SPIKE:
            LOG_I("CPU_LOAD_SPIKE");
            cpuLoadSpikeBoostScale = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_ADPF_CPU_LOAD_SPIKE_BOOST_SCALE, 0);
            cpuLoadSpikeBoostDuration = libpowerhal_wrap_UserGetCapability(MTKPOWER_CMD_GET_ADPF_CPU_LOAD_SPIKE_BOOST_DURATION, 0);
            if (cpuLoadSpikeBoostDuration == -1)
                cpuLoadSpikeBoostDuration = frameTimeMs;
            predictWorkload(this->tgid, cpuLoadSpikeBoostScale, cpuLoadSpikeBoostDuration);
            break;
        case SessionHint::GPU_LOAD_SPIKE:
            LOG_I("GPU_LOAD_SPIKE");
            break;
        default:
            LOG_E("Error: hint %d is invalid", hint);
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::setThreads(const std::vector<int32_t>& threadIds) {
    LOG_I("ADPF: tgid=%d, sid=%d", this->tgid, this->id);

    if (threadIds.size() == 0) {
        LOG_E("Error: threadIds.size() shouldn't be %d", threadIds.size());
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (int i = 0; i < threadIds.size(); i ++) {
        LOG_I("ADPF: threadIds[%d] = %d", i, threadIds[i]);
    }

    if (threadIds.size() >= ADPF_MAX_THREAD) {
        LOG_E("Error: threadIds.size %d exceeds the max size %d", threadIds.size(), ADPF_MAX_THREAD);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    _ADPF_PACKAGE msg;
    int32_t threadIds_arr[ADPF_MAX_THREAD] = {0};

    msg.cmd = ADPF::SET_THREADS;
    msg.sid = this->id;
    std::copy(threadIds.begin(), threadIds.end(), threadIds_arr);
    msg.threadIds = threadIds_arr;
    msg.threadIds_size = threadIds.size();
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}


ScopedAStatus PowerHintSession::setMode(SessionMode mode, bool enabled) {
    LOG_I("ADPF: mode=%d, enabled=%d", mode, enabled);

    switch (mode) {
        case SessionMode::POWER_EFFICIENCY:
            LOG_I("POWER_EFFICIENCY");
            break;
        case SessionMode::GRAPHICS_PIPELINE:
            LOG_I("GRAPHICS_PIPELINE");
            break;
        case SessionMode::AUTO_CPU:
            LOG_I("AUTO_CPU");
            break;
        case SessionMode::AUTO_GPU:
            LOG_I("AUTO_GPU");
            break;
        default:
            LOG_E("mode %d is invalid", mode);
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::getSessionConfig(SessionConfig* _aidl_return) {
    LOG_I("ADPF: sid = %d", this->id);
    _aidl_return->id = this->id;

    return ScopedAStatus::ok();
}



int PowerHintSession::getSessionId() {
    return this->id;
}

int PowerHintSession::getTgid() {
    return this->tgid;
}

int PowerHintSession::getUid() {
    return this->uid;
}

std::vector<int32_t> PowerHintSession::getThreadIds() {
    return this->threadIds;
}

int PowerHintSession::getDurationNanos() {
    return this->durationNanos;
}

}  // namespace aidl::android::hardware::power::impl::mediatek

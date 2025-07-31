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

namespace aidl::android::hardware::power::impl::mediatek {

using ndk::ScopedAStatus;

PowerHintSession::PowerHintSession(int32_t sessionId, int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos) {
    this->id = sessionId;
    this->tgid = tgid;
    this->uid = uid;
    this->threadIds = threadIds;
    this->durationNanos = durationNanos;

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
    LOG_I("targetDurationNanos = %ld", targetDurationNanos);
    _ADPF_PACKAGE msg;

    msg.cmd = ADPF::UPDATE_TARGET_WORK_DURATION;
    msg.sid = this->id;
    msg.targetDurationNanos = targetDurationNanos;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::reportActualWorkDuration(const std::vector<WorkDuration>& durations) {
    for (int i = 0; i < durations.size(); i ++) {
        LOG_D("durations[%d].timeStamp=%ld, durations[%d].duration=%ld", i, durations[i].timeStampNanos, i, durations[i].durationNanos);
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

    LOG_I("sid: %d", this->id);

    msg.cmd = ADPF::PAUSE;
    msg.sid = this->id;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::resume() {
    _ADPF_PACKAGE msg;

    LOG_I("sid: %d", this->id);

    msg.cmd = ADPF::RESUME;
    msg.sid = this->id;
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::close() {
    _ADPF_PACKAGE msg;

    LOG_I("sid: %d", this->id);

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

ScopedAStatus PowerHintSession::sendHint(SessionHint hint) {
    LOG_I("hint:%d", hint);
    _ADPF_PACKAGE msg;

    msg.cmd = ADPF::SENT_HINT;
    msg.sid = this->id;
    msg.hint = static_cast<int>(hint);
    adpf_sys_set_data(&msg);

    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::setThreads(const std::vector<int32_t>& threadIds) {
    if (threadIds.size() == 0) {
        LOG_E("Error: threadIds.size() shouldn't be %d", threadIds.size());
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (int i = 0; i < threadIds.size(); i ++) {
        LOG_I("threadIds[%d] = %d", i, threadIds[i]);
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

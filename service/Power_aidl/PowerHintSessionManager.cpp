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

#include "PowerHintSessionManager.h"
#include "PowerHintSession.h"
#include "include/sysctl.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::power::impl::mediatek {

int PowerHintSessionManager::addPowerSession(PowerHintSession *session) {
    LOG_I("tgid: %d, uid: %d", session->getTgid(), session->getUid());
    std::lock_guard<std::mutex> guard(mLock);
    mPowerHintSessions.insert(session);

    return -1;
}

int PowerHintSessionManager::removePowerSession(PowerHintSession *session) {
    LOG_I("tgid: %d, uid: %d", session->getTgid(), session->getUid());
    std::lock_guard<std::mutex> guard(mLock);

    int sid = session->getSessionId();

    if(sid >= 0 && sid < ADPF_MAX_SESSION) {
        sessionHintIds[sid] = 0;
    }

    return mPowerHintSessions.erase(session);
}

int PowerHintSessionManager::checkPowerSesssion(int tgid, int uid, const std::vector<int32_t>& threadIds) {
    LOG_D("tgid: %d, uid: %d", tgid, uid);
    std::lock_guard<std::mutex> guard(mLock);

    for(int i=0; i<ADPF_MAX_SESSION; i++) {
        if(sessionHintIds[i] == 0) {
            sessionHintIds[i] = 1;
            return i;
        }
    }

    return -1;
}

int PowerHintSessionManager::checkThreads(const std::vector<int32_t>& session_threadIds, const std::vector<int32_t>& threadIds) {
    for(int session_threadId : session_threadIds) {
        for(int threadId : threadIds) {
            if(session_threadId == threadId) {
                return 1;
            }
        }
    }

    return 0;
}

}  // namespace aidl::android::hardware::power::impl::mediatek

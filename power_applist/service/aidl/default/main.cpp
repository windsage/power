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

#include "mtkpower_applist.h"

using ::aidl::vendor::mediatek::hardware::mtkpower_applist::Mtkpower_applist;

int main() {
    LOG(INFO) << "start aidl Mtkpower_applist";

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Mtkpower_applist> vib = ndk::SharedRefBase::make<Mtkpower_applist>();

    const std::string instance = std::string() + Mtkpower_applist::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(vib->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE; // should not reach
}
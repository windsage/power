/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include "../../../lib/perfAPPListScenario.h"
#include "../../../../power/include/mtkpower_types.h"
#include <android-base/logging.h>
#include <log/log.h>
#include <string>
#include <dlfcn.h>
#include <pthread.h>

namespace aidl {
namespace vendor {
namespace mediatek {
namespace hardware {
namespace mtkpower_applist {

using ::ndk::ScopedAStatus;
using std::string;

#define LIB_APPLIST "lib_power_applist.so"

typedef int (*parese_app_list)(void);
typedef int (*notify_app_state)(const char*, const char*, int, int, int, int);
typedef _activity* (*get_foreground_app_info)(void);
typedef int (*check_fps_update)(_activity* );
typedef int (*set_current_mode)(const char* );
typedef int (*get_current_mode)(char*);
typedef int (*set_app_list_config)(int);
typedef void (*set_thread_hint_controller_enabled)(int);
typedef void (*reload_applist_xml)(void);
typedef int (*applist_test)(void);

static int (*parseAPPlist)(void) = NULL;
static int (*notifyAPPstate)(const char*, const char*, int, int, int, int) = NULL;
static _activity* (*getForegroundAPPInfo)(void) = NULL;
static int (*_checkFPSupdate)(_activity* ) = NULL;
static int (*setCurrentMode)(const char*) = NULL;
static int (*getCurrentMode)(char*) = NULL;
static int (*setAPPListConfig)(int) = NULL;
static void (*reloadAppListXml)(void) = NULL;
static void (*setThreadHintControllerEnabled)(int) = NULL;
static int (*applistTest)(void) = NULL;

int isAPPListSupported = 0;

int load_api()
{
    void *handle = NULL, *func = NULL;

    LOG(INFO) << "try to load APPList API";
    handle = dlopen(LIB_APPLIST, RTLD_NOW | RTLD_GLOBAL);
    if (handle == NULL) {
        LOG(ERROR) << "dlopen error: " << dlerror();
        return -1;
    }

    func = dlsym(handle, "parseAPPlist");
    parseAPPlist = reinterpret_cast<parese_app_list>(func);
    if (parseAPPlist == NULL) {
        LOG(ERROR) << "parseAPPlist failed: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "notifyAPPstate");
    notifyAPPstate = reinterpret_cast<notify_app_state>(func);
    if (notifyAPPstate == NULL) {
        LOG(ERROR) << "notifyAPPstate error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "getForegroundAPPInfo");
    getForegroundAPPInfo = reinterpret_cast<get_foreground_app_info>(func);
    if (getForegroundAPPInfo == NULL) {
        LOG(ERROR) << "getForegroundAPPInfo error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "_checkFPSupdate");
    _checkFPSupdate = reinterpret_cast<check_fps_update>(func);
    if (_checkFPSupdate == NULL) {
        LOG(ERROR) << "_checkFPSupdate error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "setCurrentMode");
    setCurrentMode = reinterpret_cast<set_current_mode>(func);
    if (setCurrentMode == NULL) {
        LOG(ERROR) << "setCurrentMode error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "getCurrentMode");
    getCurrentMode = reinterpret_cast<get_current_mode>(func);
    if (getCurrentMode == NULL) {
        LOG(ERROR) << "getCurrentMode error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "setAPPListConfig");
    setAPPListConfig = reinterpret_cast<set_app_list_config>(func);
    if (setAPPListConfig == NULL) {
        LOG(ERROR) << "setAPPListConfig error: " << dlerror();
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "reloadAppListXml");
    reloadAppListXml = reinterpret_cast<reload_applist_xml>(func);
    if (reloadAppListXml == NULL) {
        LOG_E("powerhal reloadAppListXml error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "setThreadHintControllerEnabled");
    setThreadHintControllerEnabled = reinterpret_cast<set_thread_hint_controller_enabled>(func);
    if (setThreadHintControllerEnabled == NULL) {
        LOG_E("cannot find setThreadHintControllerEnabled (dlerror=%s)", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "applistTest");
    applistTest = reinterpret_cast<applist_test>(func);
    if (applistTest == NULL) {
        LOG_E("powerhal applistTest error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    LOG(INFO) << "Load APPList API successfully.";
    return 1;
}

void* APPListFPSController(void *data)
{
    _activity* fg = NULL;
    fg = getForegroundAPPInfo();
    while(1) {
        _checkFPSupdate(fg);
    }
}

void create_fps_controller()
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&thread, &attr, APPListFPSController, NULL);
    pthread_setname_np(thread, "APPListFPS");
}

void _init()
{
    int isReady = 0;

    isAPPListSupported = load_api();
    if (isAPPListSupported == 1) {
        isReady = parseAPPlist();
    }
    if (isReady == 1) {
        create_fps_controller();
    } else {
        LOG(ERROR) << "Init MTK APPList Failed.";
    }
}

Mtkpower_applist::Mtkpower_applist() {
    _init();
}

ndk::ScopedAStatus Mtkpower_applist::notifyAppState(const std::string& packName, const std::string& actName, int pid, int state, int uid) {
	// not support

    return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus Mtkpower_applist::notifyAppStateInfo(const std::string& packName, const std::string& actName, int pid, int activityId, int state, int uid) {
	LOG(INFO) << "packName: " << packName << " actName: " << actName << " pid: " << pid << " activityId: " << activityId << " uid: " << uid << " state: " << state;

    notifyAPPstate(packName.c_str(), actName.c_str(), pid, activityId, state, uid);

    return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus Mtkpower_applist::setSysInfo(int cmd, const std::string& data) {
    LOG_D("cmd: %d, data: %s", cmd, data.c_str());
    int ret = 0;
    long int configIdx = 0;
    int control_enabled = -1;

    switch(cmd)
    {
        case SysInfo::SYSINFO_CUR_MODE:
            LOG_D("set mode: %s", data.c_str());
            setCurrentMode(data.c_str());
            break;
        case SysInfo::SYSINFO_RELOAD_APPLIST_CONFIG:
            reloadAppListXml();
            break;
        case SysInfo::SYSINFO_TEST_MODE:
            ret = applistTest();
            if(ret == -1)
                LOG_E("enter test mode fail!!");
            break;
        case SysInfo::SYSINFO_APP_LIST_CONFIG:
            LOG_D("set config: %s", data.c_str());
            configIdx = strtol(data.c_str(), NULL, 10);
            setAPPListConfig(configIdx);
            reloadAppListXml();
            break;
        case SysInfo::CONTROLLER_TYPE_APP_CFG:
            //TODO
            break;
        case SysInfo::CONTROLLER_TYPE_THREAD_HINT:
            LOG_I("set thread hint controller enabled : %s", data.c_str());
            control_enabled = strtol(data.c_str(), NULL, 10);
            setThreadHintControllerEnabled(control_enabled);
            break;
        default:
            LOG_E("UNKNOWN cmd: %d", cmd);
            break;
    }

    return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus Mtkpower_applist::getSysInfo(int cmd, std::string *_aidl_ret) {
	LOG_D("cmd: %d", cmd);

    char curMode[STR_LEN_MAX];

    switch (cmd) {
        case SysInfo::SYSINFO_CUR_MODE:
            getCurrentMode(curMode);

            *_aidl_ret = string(curMode);
            break;
        default:
            LOG_E("UNKNOWN cmd: %d", cmd);
            break;
    }

    return ndk::ScopedAStatus::ok();
};

}  // namespace mtkpower_applist
}  // namespace hardware
}  // namespace mediatek
}  // namespace vendor
}  // namespace aidl
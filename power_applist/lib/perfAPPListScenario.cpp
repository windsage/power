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

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <dlfcn.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <expat.h>
#include <pthread.h>
#include <mutex>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <cutils/compiler.h>
#include <utils/threads.h>
#include <mtkperf_resource.h>
#include "perfAPPListScenario.h"
#include "tinyxml2.h"
#include <mtkpower_hint.h>
#include <mtkpower_types.h>
//#SPD: add by rui.zhou6 by encode at 20250520 start
#include "common.h"
//#SPD: add by rui.zhou6 by encode at 20250520 end
#include <regex>
#include <errno.h>


#ifdef HAVE_AEE_FEATURE
#include "aee.h"
#endif

using namespace tinyxml2;
using namespace std;
using std::string;
using std::vector;
using std::regex;



#define DEFINE_THREAD_HINT_CMD_SETTING_FUNC(FUNC_NAME, FIELD, VALID_FIELD) \
int FUNC_NAME(const std::vector<ThreadHintPolicy>& scenarios, const ThreadHintPolicy& key) { \
    int best_specificity = -1; \
    int best_value = LOOM_CMD_DEFAULT_VALUE; \
    for (int i = 0; i < scenarios.size(); i++) { \
        const ThreadHintPolicy& s = scenarios[i]; \
        if (!s.package || !key.package || strcmp(s.package, key.package) != 0) continue; \
        if (!s.thread || !key.thread || strcmp(s.thread, key.thread) != 0) continue; \
        if (!s.VALID_FIELD) continue; \
        int cur_specificity = getFpsSpecificity(s.fps) * 10 + getWindowSpecificity(s.window); \
        if (cur_specificity > best_specificity) { \
            best_specificity = cur_specificity; \
            best_value = s.FIELD; \
        } \
    } \
    return best_value; \
}




typedef void (*fbc_get_fps)(int *, int *);
typedef int (*perf_lock_acq)(int, int, int[], int);
typedef int (*perf_lock_rel)(int);
typedef int (*powerhal_wrap_set_sys_info)(int, char *);
typedef int (*perf_cus_lock_hint)(int, int);
typedef int (*powerhal_wrap_query_sys_info)(int, int);

static int  (*querySysInfo)(int, int) = NULL;
static int  (*perfCusLockHint)(int, int) = NULL;
static int  (*setSysInfo)(int, char *) = NULL;
static int  (*perfLockAcq)(int, int, int[], int) = NULL;
static int  (*perfLockRel)(int) = NULL;
static void (*xgfGetFPS)(int *, int *) = NULL;

int disableThreadHintPolicy(int idx, int pid);
 
int isReady = 0;
int PackCount = 0;
int currentFPS = -1;
char currentMode[STR_LEN_MAX] = "LOOM";
char currentAPPListConfigPath[STR_LEN_MAX] = APP_LIST_LOOM_XMLPATH;
int isFbcSupport = -1;
int mXmlActNum = 0;
int nUserScnBase = 0;
int multi_resumed_app_count = 0;
int isMultiWindow = 0;
int AppScenarioStatusInfo[1024] = {0};
APPscenario *APPList = NULL;
_activity multi_resumed_app_info[MULTI_WIN_SIZE_MAX];
pthread_mutex_t fg_mutex = PTHREAD_MUTEX_INITIALIZER;
_activity foreground_app_info;
vector<int> applist_idx_list;
vector<vector<xml_element>> mXmlActList;
int bDuringProcessCreate = 1;
char currPackname[STR_LEN_MAX];
char currActname[STR_LEN_MAX];
int hdl_launch = 0;
int hdl_act_switch = 0;
int fg_launch_time_cold = 0;
int fg_launch_time_warm = 0;
int fg_act_switch_time = 0;
int ThreadHintPolicyCount = 0;
int isThreadHintPolicyEnabled[1024] = {0};
bool threadHintControllerEnabled = true;
vector<vector<xml_element>> XmlThreadHintList;
vector<int> currentCandidateThreadHintPolicyIdx;
vector<ThreadHintPolicy> currentCandidateThreadHintPolicy;
ThreadHintPolicy *ThreadHintCfg = NULL;


struct csv_data {
    string command_string;
    int major_id;
    int minor_id;
    int group_id;
    int command_id;
};
vector<csv_data> csv_cmd_list;


static int load_powerhal_api(void)
{
    void *handle = NULL, *func = NULL;

    LOG_I("try to load powerhal api");

    handle = dlopen(PERF_LOCK_LIB_FULL_NAME, RTLD_NOW);
    if (handle == NULL) {
        LOG_E("powerhal dlopen fail: %s\n", dlerror());
        return -1;
    }

    func = dlsym(handle, "perf_lock_acq");
    perfLockAcq = reinterpret_cast<perf_lock_acq>(func);
    if (perfLockAcq == NULL) {
        LOG_E("powerhal perfLockAcq error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "perf_cus_lock_hint");
    perfCusLockHint = reinterpret_cast<perf_cus_lock_hint>(func);
    if (perfCusLockHint == NULL) {
        LOG_E("powerhal perfCusLockHint error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "perf_lock_rel");
    perfLockRel = reinterpret_cast<perf_lock_rel>(func);
    if (perfLockRel == NULL) {
        LOG_E("powerhal perfLockRel error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    handle = dlopen(POWERHAL_WRAP_FULL_NAME, RTLD_NOW);
    if (handle == NULL) {
        LOG_E("powerhal dlopen fail: %s\n", dlerror());
        return -1;
    }

    func = dlsym(handle, "PowerHal_Wrap_querySysInfo");
    querySysInfo = reinterpret_cast<powerhal_wrap_query_sys_info>(func);
    if (querySysInfo == NULL) {
        LOG_E("powerhal querySysInfo error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "PowerHal_Wrap_setSysInfo");
    setSysInfo = reinterpret_cast<powerhal_wrap_set_sys_info>(func);
    if (setSysInfo == NULL) {
        LOG_E("powerhal setSysInfo error: %s\n", dlerror());
        dlclose(handle);
        return -1;
    }

    return 0;
}

int check_fbc_supported()
{
    void *handle = NULL, *func = NULL;

    if (isFbcSupport == 1) {
        return 1;
    } else if (isFbcSupport == -1) {
        LOG_I("try to load fpsgo fbc");
        handle = dlopen(FBC_LIB_FULL_NAME, RTLD_NOW);
        if (handle == NULL) {
            LOG_E("dlopen error: %s", dlerror());
            return -1;
        }

        func = dlsym(handle, "xgfGetFPS");
        xgfGetFPS = reinterpret_cast<fbc_get_fps>(func);
        if (xgfGetFPS == NULL) {
            LOG_E("xgfGetFPS error: %s", dlerror());
            dlclose(handle);
            return -1;
        }

        LOG_I("load fpsgo fbc successfully");
        isFbcSupport = 1;
        return 1;
    }

    return 0;
}


int getForegroundAPPCount()
{
	int i = 0, j = 0, count = 1;

	for (i = 1; i < multi_resumed_app_count; i++) {
		for (j = 0; j < i; j++) {
            if (0 == strncmp(multi_resumed_app_info[i].packName, multi_resumed_app_info[j].packName, strlen(multi_resumed_app_info[j].packName)))
				break;
        }

        if (i == j)
            count++;
	}
    LOG_I("%d", count);

    return count;
}


int writeToFile(const char* path, const char* buf, int size)
{
    if (!threadHintControllerEnabled) {
        LOG_I("skipped: threadHintController is disabled (threadHintControllerEnabled=%d)", threadHintControllerEnabled);
        return 0;
    }

    if (!path) {
        LOG_E("null path");
        return 0;
    }

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        LOG_E("cannot open file: %s", path);
        char *err_str = strerror(errno);
        LOG_E("errno=%d, strerror=%s", errno, err_str);
        return 0;
    }

    int count = write(fd, buf, size);
    if (count != size) {
        LOG_E("failed to write file! (count=%d)", count);
        char *err_str = strerror(errno);
        LOG_E("errno=%d, strerror=%s", errno, err_str);
        close(fd);
        return 0;
    }

    close(fd);

    LOG_D("%s > %s", buf, path);

    return count;
}

int getThreadHintID(char * cmdString)
{
    int i, tblLen;
    tblLen = sizeof(ThreadHintParamsMappingTbl) / sizeof(ThreadHintParams);

    for (i = 0; i < tblLen; i++) {
        if (strncmp(ThreadHintParamsMappingTbl[i].cmdString, cmdString, STR_LEN_MAX) == 0) {
            return ThreadHintParamsMappingTbl[i].cmdId;
        }
    }
    LOG_E("cannot found ThreadHint id for %s", cmdString);

    return -1;
}

void assignThreadHintParamById(ThreadHintPolicy* scenario, int cmdId, int value)
{
    switch (cmdId)
    {
        case MODE:
            scenario->mode = value;
            scenario->mode_valid = 1;
            break;
        case MATCHING_NUM:
            scenario->matching_num = value;
            scenario->matching_num_valid = 1;
            break;
        case PRIO:
            scenario->prio = value;
            scenario->prio_valid = 1;
            break;
        case CPU_MASK:
            scenario->cpu_mask = value;
            scenario->cpu_mask_valid = 1;
            break;
        case SET_EXCLUSIVE:
            scenario->set_exclusive = value;
            scenario->set_exclusive_valid = 1;
            break;
        case LOADING_UB:
            scenario->loading_ub = value;
            scenario->loading_ub_valid = 1;
            break;
        case LOADING_LB:
            scenario->loading_lb = value;
            scenario->loading_lb_valid = 1;
            break;
        case BHR:
            scenario->bhr = value;
            scenario->bhr_valid = 1;
            break;
        case LIMIT_MIN_FREQ:
            scenario->limit_min_freq = value;
            scenario->limit_min_freq_valid = 1;
            break;
        case LIMIT_MAX_FREQ:
            scenario->limit_max_freq = value;
            scenario->limit_max_freq_valid = 1;
            break;
        case SET_RESCUE:
            scenario->set_rescue = value;
            scenario->set_rescue_valid = 1;
            break;
        case RESCUE_F_OPP:
            scenario->rescue_f_opp = value;
            scenario->rescue_f_opp_valid = 1;
            break;
        case RESCUE_C_FREQ:
            scenario->rescue_c_freq = value;
            scenario->rescue_c_freq_valid = 1;
            break;
        case RESCUE_TIME:
            scenario->rescue_time = value;
            scenario->rescue_time_valid = 1;
            break;
        default:
            LOG_E("Unknown command id");
            break;
    }
}



int getIDfromString(char * input)
{
    int i = 0;

    for (i = 0; i < csv_cmd_list.size(); i++ ) {
        if (strncmp(input, csv_cmd_list[i].command_string.c_str(), PACK_NAME_MAX-1) == 0) {
            return csv_cmd_list[i].command_id;
        }
    }
    LOG_E("[cmd id not found] %s", input);
    return 0;
}

int notifyNetdUID(int uid)
{
    int ret = 0;
    char str[64] = "";

    LOG_I("%d", uid);
    if (snprintf(str, 64, "%d", uid) < 0) {
        ALOGE("%s, snprintf error", __FUNCTION__);
        return 0;
    }

    if (!setSysInfo)
        load_powerhal_api();

    ret = setSysInfo(SETSYS_NETD_SET_BOOST_UID, str);

    return 0;
}

int notifyPID(int pid)
{
    int ret = 0;
    char str[64] = "";

    LOG_I("%d", pid);
    if (snprintf(str, 64, "%d", pid) < 0) {
        ALOGE("%s, snprintf error", __FUNCTION__);
        return 0;
    }

    if (!setSysInfo)
        load_powerhal_api();

    ret = setSysInfo(SETSYS_FOREGROUND_APP_PID, str);

    return 0;
}

_activity* getForegroundAPPInfo()
{
    pthread_mutex_lock(&fg_mutex);
    LOG_I(" %s/%s (pid=%d)", foreground_app_info.packName, foreground_app_info.actName, foreground_app_info.pid);
    pthread_mutex_unlock(&fg_mutex);

    return &foreground_app_info;
}


extern "C"
int setAPPListConfig(int configIdx) {
    LOG_I("set applist config: %d", configIdx);
    pthread_mutex_lock(&fg_mutex);
    switch(configIdx)
    {
        case 0:
            strncpy(currentAPPListConfigPath, APP_LIST_XMLPATH, STR_LEN_MAX);
            break;
        case 1:
            strncpy(currentAPPListConfigPath, APP_LIST_LOOM_XMLPATH, STR_LEN_MAX);
            break;
        default:
            strncpy(currentAPPListConfigPath, APP_LIST_LOOM_XMLPATH, STR_LEN_MAX);
            break;
    }
    pthread_mutex_unlock(&fg_mutex);

    return 0;
}

extern "C"
int setCurrentMode(const char *mode) {
    char packName[PACK_NAME_MAX], actName[CLASS_NAME_MAX];
    int pid, activityId, uid;

    pthread_mutex_lock(&fg_mutex);
    if(mode != NULL && strlen(mode) > 0 && strlen(mode) < STR_LEN_MAX) {
        set_str_cpy(currentMode, mode, STR_LEN_MAX);
        LOG_I("set mode: %s", currentMode);
    }else
        LOG_E("set mode failed");

    strncpy(packName, multi_resumed_app_info[0].packName, PACK_NAME_MAX);
    packName[PACK_NAME_MAX-1] = '\0';
    strncpy(actName, multi_resumed_app_info[0].actName, CLASS_NAME_MAX);
    actName[CLASS_NAME_MAX-1] = '\0';
    pid = multi_resumed_app_info[0].pid;
    activityId = multi_resumed_app_info[0].activityId;
    uid = multi_resumed_app_info[0].uid;

    pthread_mutex_unlock(&fg_mutex);

    if(pid != -1)
        notifyAPPstate(packName, actName, pid, activityId, MODE_UPDATED, uid);

    return 0;
}

extern "C"
int getCurrentMode(char *curMode) {
    pthread_mutex_lock(&fg_mutex);
    strncpy(curMode, currentMode, STR_LEN_MAX);
    curMode[STR_LEN_MAX-1] = '\0';
    LOG_D("cur mode: %s", curMode);
    pthread_mutex_unlock(&fg_mutex);

    return 0;
}

int parse_app_scenario_from_xml(const char *path)
{
    //#SPD: add by rui.zhou6 by encode at 20250520 start
    XMLDocument docXml;
    XMLError errXml;
    char *buf_decode = NULL;
    buf_decode = get_decode_buf(path);
    errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    //errXml = docXml.LoadFile(path);
    //#SPD: add by rui.zhou6 by encode at 20250520 end
    int param_1;

    mXmlActNum = 0;

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal whitelist config file '%s'. Error: %s",
            __FUNCTION__, path, XMLDocument::ErrorIDToName(errXml));
        //#SPD: add by rui.zhou6 by encode at 20250520 start
        if(buf_decode) {
           free(buf_decode);
        }
        //#SPD: add by rui.zhou6 by encode at 20250520 end
        return 0;
    } else {
        ALOGI("%s: load powerhal CusScnTable config succeed!", __FUNCTION__);
    }

    LOG_D(" errXml:%d" , errXml);

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        LOG_E("elmtRoot == NULL !!!! NO Pachage info ");
        //#SPD: add by rui.zhou6 by encode at 20250520 start
        if(buf_decode) {
            free(buf_decode);
        }
        //#SPD: add by rui.zhou6 by encode at 20250520 end
        return 0;
    }
    XMLElement *elmtPackage = elmtRoot->FirstChildElement("Package");

    while (elmtPackage != NULL) {
        const char* packname = elmtPackage->Attribute("name");
        XMLElement *elmtActivity = elmtPackage->FirstChildElement("Activity");

        while (elmtActivity != NULL) {
            const char* actname = elmtActivity->Attribute("name");
            XMLElement *elmtMode = elmtActivity->FirstChildElement("MODE");
            XMLElement *elmtFPS = elmtActivity->FirstChildElement("FPS");

            do {
                if(elmtMode != NULL)
                    elmtFPS = elmtMode->FirstChildElement("FPS");

                while (elmtFPS != NULL) {
                    const char* mode = (elmtMode != NULL) ? elmtMode->Attribute("mode") : "LOOM";
                    const char* FPSvalue = elmtFPS->Attribute("value");
                    const char* sbefeaturename = elmtFPS->Attribute("type");
                    XMLElement *elmtWINDOW = elmtFPS->FirstChildElement("WINDOW");
                    while (elmtWINDOW != NULL) {
                        const char* WINDOWmode = elmtWINDOW->Attribute("mode");
                        XMLElement *dataelmt = elmtWINDOW->FirstChildElement("data");
                        vector<xml_element> row;

                        while (dataelmt != NULL) {
                            xml_element temp;
                            const char* cmdname = dataelmt->Attribute("cmd");
                            param_1 = dataelmt->IntAttribute("param1");
                            if(packname != NULL && strlen(packname) < PACK_NAME_MAX)
                                set_str_cpy(temp.packName, packname, PACK_NAME_MAX);
                            if(actname != NULL && strlen(actname) < CLASS_NAME_MAX)
                                set_str_cpy(temp.actName, actname, CLASS_NAME_MAX);
                            if(elmtMode != NULL && mode != NULL && strlen(mode) < CLASS_NAME_MAX)
                                set_str_cpy(temp.mode, mode, CLASS_NAME_MAX);
                            if(FPSvalue != NULL && strlen(FPSvalue) < CLASS_NAME_MAX)
                                set_str_cpy(temp.fps, FPSvalue, CLASS_NAME_MAX);
                            if(WINDOWmode != NULL && strlen(WINDOWmode) < CLASS_NAME_MAX)
                                set_str_cpy(temp.window_mode, WINDOWmode, CLASS_NAME_MAX);
                            if(sbefeaturename != NULL && strlen(sbefeaturename) < PACK_NAME_MAX) {
                                set_str_cpy(temp.sbefeatureName, sbefeaturename, PACK_NAME_MAX);
                            } else {
                                set_str_cpy(temp.sbefeatureName, "\0", 1);
                            }
                            if(cmdname != NULL && strlen(cmdname) < 128)
                                set_str_cpy(temp.cmd, cmdname, 128);
                            temp.param1 = param_1;
                            row.push_back(temp);
                            LOG_D("pack:%s ack:%s mode:%s fps:%s window:%s type:%s (%s, %d)",
                                packname, actname, mode, FPSvalue, WINDOWmode, sbefeaturename, cmdname, param_1);
                            dataelmt = dataelmt->NextSiblingElement();
                        }
                        mXmlActList.push_back(row);
                        mXmlActNum++;

                        elmtWINDOW = elmtWINDOW->NextSiblingElement();
                    }
                    elmtFPS = elmtFPS->NextSiblingElement();
                }
                if(elmtMode != NULL)
                    elmtMode = elmtMode->NextSiblingElement();

            } while (elmtMode != NULL);
            elmtActivity = elmtActivity->NextSiblingElement();
        }
        elmtPackage = elmtPackage->NextSiblingElement();
    }
    LOG_I("applist scenario count = %d\n", mXmlActNum);
    //#SPD: add by rui.zhou6 by encode at 20250520 start
    if(buf_decode) {
        free(buf_decode);
    }
    //#SPD: add by rui.zhou6 by encode at 20250520 end
    return mXmlActNum;
}

int parseThreadHintCfgXml(const char *path)
{
    XMLDocument docXml;
    XMLError errXml = docXml.LoadFile(path);
    LOG_I("errXml:%d", errXml);
    int param_1;

    int ThreadHintPolicyCount = 0;

    if (errXml != XML_SUCCESS) {
        LOG_E("failed to load file %s (error: %s)", path, XMLDocument::ErrorIDToName(errXml));
        return 0;
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        LOG_E("elmtRoot == NULL");
        return 0;
    }

    XMLElement *elmtPackage = elmtRoot->FirstChildElement("Package");
    while (elmtPackage != NULL) {
        const char* packname = elmtPackage->Attribute("name");
        XMLElement *elmtFPS = elmtPackage->FirstChildElement("FPS");
        while (elmtFPS != NULL) {
            const char* FPSvalue = elmtFPS->Attribute("value");
            XMLElement *elmtWINDOW = elmtFPS->FirstChildElement("WINDOW");
            while (elmtWINDOW != NULL) {
                const char* WINDOWmode = elmtWINDOW->Attribute("mode");
                XMLElement *elmtThread = elmtWINDOW->FirstChildElement("Thread");
                while (elmtThread != NULL) {
                    const char* ThreadName = elmtThread->Attribute("name");
                    XMLElement *dataelmt = elmtThread->FirstChildElement("data");
                    vector<xml_element> row;
                    while (dataelmt != NULL) {
                        xml_element temp;
                        const char* cmdname = dataelmt->Attribute("cmd");
                        param_1 = dataelmt->IntAttribute("param1");
                        if (packname != NULL && strlen(packname) < STR_LEN_MAX)
                            set_str_cpy(temp.packName, packname, STR_LEN_MAX);
                        if (FPSvalue != NULL && strlen(FPSvalue) < STR_LEN_MAX)
                            set_str_cpy(temp.fps, FPSvalue, STR_LEN_MAX);
                        if (WINDOWmode != NULL && strlen(WINDOWmode) < STR_LEN_MAX)
                            set_str_cpy(temp.window_mode, WINDOWmode, STR_LEN_MAX);
                        if (ThreadName != NULL && strlen(ThreadName) < STR_LEN_MAX)
                            set_str_cpy(temp.threadName, ThreadName, STR_LEN_MAX);
                        if (cmdname != NULL && strlen(cmdname) < STR_LEN_MAX)
                            set_str_cpy(temp.cmd, cmdname, STR_LEN_MAX);
                        temp.param1 = param_1;
                        row.push_back(temp);

                        LOG_I("package=%s, fps=%s, window=%s, thread=%s, cmd=%s, param=%d",
                            packname, FPSvalue, WINDOWmode, ThreadName, cmdname, param_1);

                        dataelmt = dataelmt->NextSiblingElement();
                    }
                    elmtThread = elmtThread->NextSiblingElement();

                    XmlThreadHintList.push_back(row);
                    ThreadHintPolicyCount++;
                }
                elmtWINDOW = elmtWINDOW->NextSiblingElement();
            }
            elmtFPS = elmtFPS->NextSiblingElement();
        }
        elmtPackage = elmtPackage->NextSiblingElement();
    }

    LOG_I("ThreadHintPolicyCount = %d", ThreadHintPolicyCount);

    return ThreadHintPolicyCount;
}

int xmlparse()
{
    int i = 0, j = 0;
    const char * file_path = NULL;

    PackCount = 0;

    if(APPList != NULL) {
        free(APPList);
        APPList = NULL;
    }

    if (access(currentAPPListConfigPath, F_OK) != -1) {
        file_path = currentAPPListConfigPath;
    } else if (access(APP_LIST_XMLPATH, F_OK) != -1) {
        file_path = APP_LIST_XMLPATH;
    } else {
        file_path = NULL;
        LOG_E("cannot find power app config !!!");
        return 0;
    }

    PackCount = parse_app_scenario_from_xml(file_path);

    if (PackCount == 0) {
        LOG_E("xml parse fail");
    } else if (PackCount >= 0) {
        if ((APPList = (APPscenario*)malloc(sizeof(APPscenario) * (PackCount))) == NULL) {
            ALOGE("Can't allocate memory for APPList");
            return 0;
        }
    }
    memset(APPList, 0, sizeof(APPscenario)*(PackCount));

    if(APPList == NULL) {
        LOG_E("APPList is null!!");
        return 0;
    }

    for (i = 0; i < mXmlActList.size(); i++) {
        vector<xml_element>::iterator iter = mXmlActList[i].begin();

        LOG_D("pack:%s ack:%s", iter->packName, iter->actName);

        APPList[i].pack_name[0] = '\0';
        APPList[i].act_name[0]  = '\0';
        APPList[i].mode[0]  = '\0';
        APPList[i].fps[0]  = '\0';
        APPList[i].window_mode[0]  = '\0';
        APPList[i].sbe_featurename[0]  = '\0';
        set_str_cpy(APPList[i].pack_name, iter->packName, PACK_NAME_MAX);
        set_str_cpy(APPList[i].act_name, iter->actName, CLASS_NAME_MAX);
        set_str_cpy(APPList[i].mode, iter->mode, CLASS_NAME_MAX);
        set_str_cpy(APPList[i].sbe_featurename, iter->sbefeatureName, PACK_NAME_MAX);
        set_str_cpy(APPList[i].fps, iter->fps, PACK_NAME_MAX);
        set_str_cpy(APPList[i].window_mode, iter->window_mode, PACK_NAME_MAX);

        int len = 0;
        for (j = 0; iter != mXmlActList[i].end(); iter++, j++) {
            LOG_D("scn:%d pack:%s ack:%s mode:%s fps:%s window:%s type:%s cmd:%s param:%d",
                i, iter->packName, iter->actName, iter->mode, iter->fps, iter->window_mode, iter->sbefeatureName, iter->cmd, iter->param1);

            APPList[i].lock_rsc_list[j*2] = getIDfromString(iter->cmd);
            APPList[i].lock_rsc_list[j*2+1] = iter->param1;
            len = len + 2;
        }
        APPList[i].list_len = len;
    }

    if (PackCount >= 0) {
        LOG_I("applist scn count = %d", PackCount);
        LOG_D("dump applist :");
        for (i = 0; i < PackCount; i++) {
            LOG_D("pack:%s act:%s mode:%s fps:%s window:%s len=%d",
                APPList[i].pack_name, APPList[i].act_name, APPList[i].mode, APPList[i].fps, APPList[i].window_mode, APPList[i].list_len);
            for (j = 0; j < APPList[i].list_len; j = j+2) {
                LOG_D("(%d, %d)", APPList[i].lock_rsc_list[j], APPList[i].lock_rsc_list[j+1]);
            }
        }
    } else {
        LOG_E("invalid PackCount %d ", PackCount);
        free(APPList);
        APPList = NULL;
        return 0;
    }

    LOG_I("start free List");
    if(mXmlActList.size() > 0) {
        mXmlActList.clear();
        mXmlActList.shrink_to_fit();
    }
    LOG_I("parse power_app_cfg done.");


    // start to parse thread hint xml
    // disable all thread hint policy
    for (i = 0; i< ThreadHintPolicyCount; i++) {
        LOG_I("disable ThreadHint policy: %d, packName: %s, pid: %d", i, ThreadHintCfg[i].package, ThreadHintCfg[i].pid);
        disableThreadHintPolicy(i, ThreadHintCfg[i].pid);
    }

    if(ThreadHintCfg != NULL) {
        free(ThreadHintCfg);
        ThreadHintCfg = NULL;
    }

    XmlThreadHintList.clear();

    if (access(THREAD_HINT_CFG_XML_PATH, F_OK) != -1) {
        file_path = THREAD_HINT_CFG_XML_PATH;
    } else {
        file_path = NULL;
        LOG_E("cannot find %s", THREAD_HINT_CFG_XML_PATH);
        return 1;
    }

    ThreadHintPolicyCount = parseThreadHintCfgXml(file_path);
    LOG_I("ThreadHintPolicyCount = %d", ThreadHintPolicyCount);

    if (ThreadHintPolicyCount > 0) {
        if ((ThreadHintCfg = (ThreadHintPolicy*)malloc(sizeof(ThreadHintPolicy) * (ThreadHintPolicyCount))) == NULL) {
            LOG_E("Cannot allocate memory for ThreadHintCfg");
            return 1;
        }
    } else {
        return 1;
    }
    memset(ThreadHintCfg, 0, sizeof(ThreadHintPolicy)*(ThreadHintPolicyCount));

    for (i = 0; i < XmlThreadHintList.size(); i++) {
        vector<xml_element>::iterator iter = XmlThreadHintList[i].begin();

        ThreadHintCfg[i].package[0] = '\0';
        ThreadHintCfg[i].fps[0]  = '\0';
        ThreadHintCfg[i].window[0]  = '\0';
        ThreadHintCfg[i].thread[0]  = '\0';

        set_str_cpy(ThreadHintCfg[i].package, iter->packName, STR_LEN_MAX);
        set_str_cpy(ThreadHintCfg[i].fps, iter->fps, STR_LEN_MAX);
        set_str_cpy(ThreadHintCfg[i].window, iter->window_mode, STR_LEN_MAX);
        set_str_cpy(ThreadHintCfg[i].thread, iter->threadName, STR_LEN_MAX);

        for (j = 0; iter != XmlThreadHintList[i].end(); iter++, j++) {
            LOG_I("idx=%d, pack=%s, fps=%s, window=%s, thread=%s, cmd=%s, param=%d",
                i, iter->packName, iter->fps, iter->window_mode, iter->threadName, iter->cmd, iter->param1);

            assignThreadHintParamById(&ThreadHintCfg[i], getThreadHintID(iter->cmd), iter->param1);
        }
    }

    if (ThreadHintPolicyCount < 0) {
        LOG_E("invalid ThreadHintPolicyCount: %d", ThreadHintPolicyCount);
        free(ThreadHintCfg);
        ThreadHintCfg = NULL;
        return 1;
    }

    LOG_I("parse thread hint xml done.");

    return 1;
}


int _getFPSvalue(int *pid, int test_mode)
{
    int dfrc_fps = 0;
    int fps30_tolerance = 30 + 30*FPS_TOLERANCE_PERCENT/100;
    int fps45_tolerance = 45 + 45*FPS_TOLERANCE_PERCENT/100;
    int fps60_tolerance = 60 + 60*FPS_TOLERANCE_PERCENT/100;
	int fps90_tolerance = 90 + 90*FPS_TOLERANCE_PERCENT/100;
	int fps120_tolerance = 120 + 120*FPS_TOLERANCE_PERCENT/100;
	int fps144_tolerance = 144 + 144*FPS_TOLERANCE_PERCENT/100;
	int get_currentFPS = -1;

    if (check_fbc_supported() != 1) {
        return 0;
    }

    if(!test_mode)
        xgfGetFPS(pid, &dfrc_fps);

    if (0 < dfrc_fps && dfrc_fps <= fps30_tolerance) {
        get_currentFPS = 30;
    } else if (fps30_tolerance < dfrc_fps && dfrc_fps <= fps45_tolerance) {
        get_currentFPS = 45;
    } else if (fps45_tolerance < dfrc_fps && dfrc_fps <= fps60_tolerance) {
        get_currentFPS = 60;
    } else if (fps60_tolerance < dfrc_fps && dfrc_fps <= fps90_tolerance) {
        get_currentFPS = 90;
    } else if (fps90_tolerance < dfrc_fps && dfrc_fps <= fps120_tolerance) {
        get_currentFPS = 120;
    } else if (fps120_tolerance < dfrc_fps && dfrc_fps <= fps144_tolerance) {
        get_currentFPS = 144;
    } else {
        get_currentFPS = -1;
    }

    LOG_D("pid(%d) - xgfGetFPS: %d, currentFPS: %d", *pid, dfrc_fps, get_currentFPS);
    return get_currentFPS;
}

int _checkFPSupdate(_activity* fg)
{
    int pid = -1, activityId = -1, uid = -1, fps_mode = -1, notified = 0;
    char packName[PACK_NAME_MAX], actName[CLASS_NAME_MAX];

    fps_mode = _getFPSvalue(&pid, 0);

    pthread_mutex_lock(&fg_mutex);
    for (int i = 0; i < multi_resumed_app_count; i++) {
        if (multi_resumed_app_info[i].fps != fps_mode && multi_resumed_app_info[i].pid == pid) {
            ALOGI("[updateFPS] pid(%d) : %d => %d", pid, multi_resumed_app_info[i].fps, fps_mode);
            currentFPS = fps_mode;
            strncpy(packName, multi_resumed_app_info[i].packName, PACK_NAME_MAX-1);
            packName[PACK_NAME_MAX-1] = '\0';
            strncpy(actName, multi_resumed_app_info[i].actName, CLASS_NAME_MAX-1);
            actName[CLASS_NAME_MAX-1] = '\0';
            activityId = multi_resumed_app_info[i].activityId;
            uid = multi_resumed_app_info[i].uid;
            notified = 1;
            break;
        }
    }
    pthread_mutex_unlock(&fg_mutex);

    if(notified == 1)
        notifyAPPstate(packName, actName, pid, activityId, FPS_UPDATED, uid);

    return 0;
}

int parseCSV()
{
    int i = 0;
    ifstream inputFile;
    inputFile.open(CMD_CSV_PATH);
    string line = "";
    
    while (getline(inputFile, line)) {
        stringstream inputString(line);

        //COMMAND, MAJOR, MINOR, GROUP
        string command_string;
        int major_id = 0;
        int minor_id = 0;
        int group_id = 0;
        int command_id = 0;
        errno = 0;
        string tempString;

        getline(inputString, command_string, ',');
        getline(inputString, tempString, ',');
        major_id = strtol(tempString.c_str(), NULL, 10);
        if (errno != 0) {
            // Handle the error: value out of range
            LOG_E("major_id out of range: %s", tempString.c_str());
            return -1;
        }

        errno = 0;
        getline(inputString, tempString, ',');
        minor_id = strtol(tempString.c_str(), NULL, 10);
        if (errno != 0) {
            // Handle the error: value out of range
            LOG_E("minor_id out of range: %s", tempString.c_str());
            return -1;
        }

        errno = 0;
        getline(inputString, tempString, ',');
        group_id = strtol(tempString.c_str(), NULL, 10);
        if (errno != 0) {
            // Handle the error: value out of range
            LOG_E("group_id out of range: %s", tempString.c_str());
            return -1;
        }

        command_id = (major_id << 22 | minor_id << 14 | group_id << 8);

        csv_data tempCsvData;
        tempCsvData.command_string = std::move(command_string);
        tempCsvData.major_id = major_id;
        tempCsvData.minor_id = minor_id;
        tempCsvData.group_id = group_id;
        tempCsvData.command_id = command_id;

        csv_cmd_list.push_back(tempCsvData);
        line = "";
    }

    LOG_D("dump csv command list: ");
    for (i = 0; i < csv_cmd_list.size(); i++ ) {
        LOG_D("%s, %d, %d, %d, %x", csv_cmd_list[i].command_string.c_str(), csv_cmd_list[i].major_id, csv_cmd_list[i].minor_id, csv_cmd_list[i].group_id, csv_cmd_list[i].command_id);
    }

    return 0;
}

extern "C"
int parseAPPlist(void)
{
    LOG_I("start parsing command.csv");
    parseCSV();

    return xmlparse();
}






/**
 * For candidates with the parameter set, calculate a "specificity" score based on FPS and window mode.
 * Among all candidates, select the value from the scenario with the highest specificity.
 * Higher specificity means the scenario is more specific (e.g., exact FPS and window match)
 */
int getFpsSpecificity(const char* fps)
{
    if (strncmp(fps, "60", 2) == 0 || strncmp(fps, "90", 2) == 0 || strncmp(fps, "120", 3) == 0)
        return FPS_SPECIFIED;
    if (strncmp(fps, "Common", 6) == 0)
        return FPS_COMMON;
    return 0;
}

int getWindowSpecificity(const char* window)
{
    if (strncmp(window, "Single", 6) == 0 || strncmp(window, "Multi", 5) == 0)
        return WINDOW_SPECIFIED;
    if (strncmp(window, "Common", 6) == 0)
        return WINDOW_COMMON;
    return 0;
}



DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_mode,           mode,           mode_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_matching_num,   matching_num,   matching_num_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_prio,           prio,           prio_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_cpu_mask,       cpu_mask,       cpu_mask_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_set_exclusive,  set_exclusive,  set_exclusive_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_loading_ub,     loading_ub,     loading_ub_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_loading_lb,     loading_lb,     loading_lb_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_bhr,            bhr,            bhr_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_limit_min_freq, limit_min_freq, limit_min_freq_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_limit_max_freq, limit_max_freq, limit_max_freq_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_set_rescue,     set_rescue,     set_rescue_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_rescue_f_opp,   rescue_f_opp,   rescue_f_opp_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_rescue_c_freq,  rescue_c_freq,  rescue_c_freq_valid)
DEFINE_THREAD_HINT_CMD_SETTING_FUNC(decide_final_rescue_time,    rescue_time,    rescue_time_valid)


/**
 * For each parameter (e.g., mode, matching_num, etc.) of a given scenario (identified by package, thread, etc.),
 * it selects the most appropriate value from all available candidate scenarios.
 */
unsigned int decideFinalThreadHintPolicy(const std::vector<ThreadHintPolicy>& input)
{
    vector<ThreadHintPolicy> output;

    for (int i = 0; i < input.size(); i++) {
        const ThreadHintPolicy& scenario = input[i];
        bool isSamePackageAndThread = false;

        for (int j = 0; j < output.size(); j++) {
            char temp_package[STR_LEN_MAX];
            char temp_thread[STR_LEN_MAX];
            strncpy(temp_package, output[j].package, sizeof(temp_package));
            temp_package[sizeof(temp_package) - 1] = '\0';
            strncpy(temp_thread, output[j].thread, sizeof(temp_thread));
            temp_thread[sizeof(temp_thread) - 1] = '\0';
            
            if ((strcmp(scenario.package, temp_package) == 0) &&
                (strcmp(scenario.thread, temp_thread) == 0)) {
                isSamePackageAndThread = true;
                break;
            }
        }

        if (!isSamePackageAndThread) {
            ThreadHintPolicy merged = scenario;
            output.push_back(merged);
        }

    }

    LOG_D("output.size()=%zu", output.size());

    for (int i = 0; i < output.size(); i++) {
        ThreadHintPolicy& out_scenario = output[i];
        out_scenario.mode            = decide_final_mode(input, out_scenario);
        out_scenario.matching_num    = decide_final_matching_num(input, out_scenario);
        out_scenario.prio            = decide_final_prio(input, out_scenario);
        out_scenario.cpu_mask        = decide_final_cpu_mask(input, out_scenario);
        out_scenario.set_exclusive   = decide_final_set_exclusive(input, out_scenario);
        out_scenario.loading_ub      = decide_final_loading_ub(input, out_scenario);
        out_scenario.loading_lb      = decide_final_loading_lb(input, out_scenario);
        out_scenario.bhr             = decide_final_bhr(input, out_scenario);
        out_scenario.set_rescue      = decide_final_set_rescue(input, out_scenario);
        out_scenario.limit_min_freq  = decide_final_limit_min_freq(input, out_scenario);
        out_scenario.limit_max_freq  = decide_final_limit_max_freq(input, out_scenario);
        out_scenario.rescue_f_opp    = decide_final_rescue_f_opp(input, out_scenario);
        out_scenario.rescue_c_freq   = decide_final_rescue_c_freq(input, out_scenario);
        out_scenario.rescue_time     = decide_final_rescue_time(input, out_scenario);
    }

    for (int i = 0; i < output.size(); i++) {
        LOG_I("i=%d, package=%s, pid=%d, thread=%s", i, output[i].package, output[i].pid, output[i].thread);
        LOG_I("mode=%d, matching_num=%d, prio=%d, cpu_mask=%d, set_exclusive=%d, "
              "loading_ub=%d, loading_lb=%d, bhr=%d, limit_min_freq=%d, limit_max_freq=%d, "
              "set_rescue=%d, rescue_f_opp=%d, rescue_c_freq=%d, rescue_time=%d",
              output[i].mode, output[i].matching_num, output[i].prio, output[i].cpu_mask, output[i].set_exclusive,
              output[i].loading_ub, output[i].loading_lb, output[i].bhr, output[i].limit_min_freq, output[i].limit_max_freq,
              output[i].set_rescue, output[i].rescue_f_opp, output[i].rescue_c_freq, output[i].rescue_time);
    }

    for (int i = 0; i < output.size(); i++) {
        char outBuffer[1024];
        const ThreadHintPolicy& s = output[i];

        outBuffer[0] = '\0';

        char _pid[STR_LEN_MAX];
        if (snprintf(_pid, sizeof(_pid), "%d ", output[i].pid) < 0)
            LOG_E("snprintf error");
        strncat(outBuffer, _pid, strlen(_pid));

        char _thread[STR_LEN_MAX];
        if (snprintf(_thread, sizeof(_thread), "%s ", s.thread) < 0)
            LOG_E("snprintf error");
        strncat(outBuffer, _thread, strlen(_thread));

        char _tid[STR_LEN_MAX];
        int tid = -1;
        if (snprintf(_tid, sizeof(_tid), "%d ", tid) < 0)
            LOG_E("snprintf error");
        strncat(outBuffer, _tid, strlen(_tid));

        int param_values[THREAD_HINT_PARAM_NUM] = {
            s.mode,
            s.matching_num,
            s.prio,
            s.cpu_mask,
            s.set_exclusive,
            s.loading_ub,
            s.loading_lb,
            s.bhr,
            s.limit_min_freq,
            s.limit_max_freq,
            s.set_rescue,
            s.rescue_f_opp,
            s.rescue_c_freq,
            s.rescue_time
        };

        for (int j = 0; j < THREAD_HINT_PARAM_NUM; j++) {
            int v = param_values[j];
            char _cmd[STR_LEN_MAX];
            if (snprintf(_cmd, sizeof(_cmd), "%d ", v) < 0)
                LOG_E("snprintf error");
            strncat(outBuffer, _cmd, strlen(_cmd));
        }

        writeToFile(SYSFS_GAME_LOOM_TASK_CFG_PATH, outBuffer, strlen(outBuffer));
    }

    return output.size();
}

int disableThreadHintPolicy(int idx, int pid)
{
    LOG_I("idx=%d, pid=%d, package=%s, fps=%s, window=%s, thread=%s",
        idx, pid, ThreadHintCfg[idx].package, ThreadHintCfg[idx].fps, ThreadHintCfg[idx].window, ThreadHintCfg[idx].thread);

    char outBuffer[1024];
    ThreadHintPolicy s = ThreadHintCfg[idx];

    outBuffer[0] = '\0';

    char _pid[STR_LEN_MAX];
    if (snprintf(_pid, sizeof(_pid), "%d ", pid) < 0)
        LOG_E("snprintf error");
    strncat(outBuffer, _pid, strlen(_pid));

    char _thread[STR_LEN_MAX];
    if (snprintf(_thread, sizeof(_thread), "%s ", s.thread) < 0)
        LOG_E("snprintf error");
    strncat(outBuffer, _thread, strlen(_thread));

    char _tid[STR_LEN_MAX];
    int tid = -1;
    if (snprintf(_tid, sizeof(_tid), "%d ", tid) < 0)
        LOG_E("snprintf error");
    strncat(outBuffer, _tid, strlen(_tid));

    for (int i = 0; i < THREAD_HINT_PARAM_NUM; i++) {
        int v = LOOM_CMD_DEFAULT_VALUE;
        char _cmd[STR_LEN_MAX];
        if (snprintf(_cmd, sizeof(_cmd), "%d ", v) < 0)
            LOG_E("snprintf error");
        strncat(outBuffer, _cmd, strlen(_cmd));
    }

    writeToFile(SYSFS_GAME_LOOM_TASK_CFG_PATH, outBuffer, strlen(outBuffer));


    return 0;
}

void addCandidateThreadHintPolicyIdx(int i)
{
    currentCandidateThreadHintPolicyIdx.push_back(i);
    LOG_D("i=%d, package=%s, fps=%s, window=%s, thread=%s",
        i, ThreadHintCfg[i].package, ThreadHintCfg[i].fps, ThreadHintCfg[i].window, ThreadHintCfg[i].thread);
}

void findThreadHintPolicyIdx(const char *packName, ThreadHintPolicy *ThreadHintCfg, int currentFPS, int is_multi_window)
{
    for (int i = 0; i < ThreadHintPolicyCount; i++) {
        if (0 == strncmp(ThreadHintCfg[i].package, packName, strlen(packName))) {
            if (strtol(ThreadHintCfg[i].fps, NULL, 10) == currentFPS) {
                if (0 == strncmp(ThreadHintCfg[i].window, "Common", STR_LEN_MAX)) {
                    addCandidateThreadHintPolicyIdx(i);
                }

                if (0 == strncmp(ThreadHintCfg[i].window, "Single", STR_LEN_MAX) && is_multi_window == 0) {
                    addCandidateThreadHintPolicyIdx(i);
                } else if (0 == strncmp(ThreadHintCfg[i].window, "Multi", STR_LEN_MAX) && is_multi_window == 1) {
                    addCandidateThreadHintPolicyIdx(i);
                }
            }

            if (0 == strncmp(ThreadHintCfg[i].fps, "Common", STR_LEN_MAX)) {
                if (0 == strncmp(ThreadHintCfg[i].window, "Common", STR_LEN_MAX)) {
                    addCandidateThreadHintPolicyIdx(i);
                }

                if (0 == strncmp(ThreadHintCfg[i].window, "Single", STR_LEN_MAX) && is_multi_window == 0) {
                    addCandidateThreadHintPolicyIdx(i);
                } else if (0 == strncmp(ThreadHintCfg[i].window, "Multi", STR_LEN_MAX) && is_multi_window == 1) {
                    addCandidateThreadHintPolicyIdx(i);
                }
            }

        }
    }

}





int AppScenarioEnable(int idx, int pid)
{
    int i = 0, j = 0;
    int perf_lock_rsc[MAX_ARGS_PER_REQUEST];
    int hdl = 0;
    int duration = 0;
    int size = APPList[idx].list_len;

    LOG_I("idx:%d, pack:%s, act:%s, fps:%s, window:%s !!!", idx, APPList[idx].pack_name, APPList[idx].act_name, APPList[idx].fps, APPList[idx].window_mode);

    if (!perfLockAcq || !perfLockRel)
        load_powerhal_api();

    for (i = 0; i < APPList[idx].list_len; i++) {
        perf_lock_rsc[i] = APPList[idx].lock_rsc_list[i];

        if (APPList[idx].lock_rsc_list[i] == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_COLD)
            fg_launch_time_cold = APPList[idx].lock_rsc_list[i+1];
        else if (APPList[idx].lock_rsc_list[i] == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_WARM)
            fg_launch_time_warm = APPList[idx].lock_rsc_list[i+1];
        else if (APPList[idx].lock_rsc_list[i] == PERF_RES_POWERHAL_WHITELIST_ACT_SWITCH_TIME)
            fg_act_switch_time = APPList[idx].lock_rsc_list[i+1];
    }
    if (fg_launch_time_cold > 0 || fg_launch_time_warm > 0 || fg_act_switch_time > 0)
        LOG_I("launch cold:%d warm:%d act:%d", fg_launch_time_cold, fg_launch_time_warm, fg_act_switch_time);

    // add fpsgo render cmd
    perf_lock_rsc[size] = PERF_RES_FPS_FPSGO_RENDER_PID;
    perf_lock_rsc[size+1] = pid;
    size = size + 2;

    hdl = perfLockAcq(0, duration, perf_lock_rsc, size);
    APPList[idx].handle_idx = hdl;

    return 0;
}

int AppScenarioDisable(int idx)
{
    int hdl = 0;
    hdl = APPList[idx].handle_idx;
    APPList[idx].pid = 0;

    if (!perfLockAcq || !perfLockRel)
        load_powerhal_api();

    if (hdl > 0 ) {
        perfLockRel(hdl);
        APPList[idx].handle_idx = 0;
    }

    return 0;
}

string escape(const char* str) {
    const static std::regex special_chars { R"([-[\]{}()*+?.,\^$|#\s])" };

    return std::regex_replace(str, special_chars, R"(\$&)");
}

bool isSamePattern(const char* pattern, const char* str) {
    if (pattern == NULL || str == NULL) return false;
    string safePattern = escape(pattern);
    string regPattern = "^" + std::regex_replace(safePattern, regex(R"(\\\*)"), R"(.*)") + "$";
    regex reg(regPattern);

    return regex_match(str, reg);
}






int findAppScenarioIndex(const char *packName, const char *actName, APPscenario *APPList, int mycurrentFPS,
                        int is_multi_window, const char * mode)
{
    int i, j;
    int Act_Match = 0;
    int Pack_Match = 0;
    int act_common_fps_common_window_common_idx = -1;
    int act_common_fps_match_window_common_idx = -1;
    int act_match_fps_common_window_common_idx = -1;
    int act_match_fps_match_window_common_idx = -1;
    int act_common_fps_common_window_match_idx = -1;
    int act_common_fps_match_window_match_idx = -1;
    int act_match_fps_common_window_match_idx = -1;
    int act_match_fps_match_window_match_idx = -1;


    for (i = 0; i < PackCount; i++) {
        if (isSamePattern(APPList[i].pack_name, packName)) {
            Pack_Match = 1;
            if (isSamePattern(APPList[i].act_name, actName)) {
                Act_Match = 1;
                if(0 == strcmp(APPList[i].mode, mode) || 0 == strcmp(APPList[i].mode, "")) {
                    if (strtol(APPList[i].fps, NULL, 10) == mycurrentFPS) {
                        if (0 == strncmp(APPList[i].window_mode, "Common", 6)) {
                            act_match_fps_match_window_common_idx = i;
                        }
                        if (0 == strncmp(APPList[i].window_mode, "Single", 6) && is_multi_window == 0) {
                            act_match_fps_match_window_match_idx = i;
                        } else if (0 == strncmp(APPList[i].window_mode, "Multi", 5) && is_multi_window == 1) {
                            act_match_fps_match_window_match_idx = i;
                        }
                    }
                    if (0 == strncmp(APPList[i].fps, "Common", 6)) {
                        if (0 == strncmp(APPList[i].window_mode, "Common", 6)) {
                            act_match_fps_common_window_common_idx = i;
                        }
                        if (0 == strncmp(APPList[i].window_mode, "Single", 6) && is_multi_window == 0) {
                            act_match_fps_common_window_match_idx = i;
                        } else if (0 == strncmp(APPList[i].window_mode, "Multi", 5) && is_multi_window == 1) {
                            act_match_fps_common_window_match_idx = i;
                        }
                    }
                }
            } else if (0 == strncmp(APPList[i].act_name, "Common", 6)) {
                if(0 == strcmp(APPList[i].mode, mode) || 0 == strcmp(APPList[i].mode, "")) {
                    if (strtol(APPList[i].fps, NULL, 10) == mycurrentFPS) {
                        if (0 == strncmp(APPList[i].window_mode, "Common", 6)) {
                            act_common_fps_match_window_common_idx = i;
                        }
                        if (0 == strncmp(APPList[i].window_mode, "Single", 6) && is_multi_window == 0) {
                            act_common_fps_match_window_match_idx = i;
                        } else if (0 == strncmp(APPList[i].window_mode, "Multi", 5) && is_multi_window == 1) {
                            act_common_fps_match_window_match_idx = i;
                        }
                    }
                    if (0 == strncmp(APPList[i].fps, "Common", 6)) {
                        if (0 == strncmp(APPList[i].window_mode, "Common", 6)) {
                            act_common_fps_common_window_common_idx = i;
                        }
                        if (0 == strncmp(APPList[i].window_mode, "Single", 6) && is_multi_window == 0) {
                            act_common_fps_common_window_match_idx = i;
                        } else if (0 == strncmp(APPList[i].window_mode, "Multi", 5) && is_multi_window == 1) {
                            act_common_fps_common_window_match_idx = i;
                        }
                    }
                }
            }
        }
    }
    ALOGD("[perfNotifyAppState][FindScenarioIndex] %d, %d, %d, %d, %d, %d, %d, %d",
        act_common_fps_common_window_common_idx, act_common_fps_match_window_common_idx, act_match_fps_common_window_common_idx, act_match_fps_match_window_common_idx,
        act_common_fps_common_window_match_idx, act_common_fps_match_window_match_idx, act_match_fps_common_window_match_idx, act_match_fps_match_window_match_idx);

    if (Pack_Match) {
        if ((act_common_fps_common_window_common_idx != -1))
            applist_idx_list.push_back(act_common_fps_common_window_common_idx);
        if ((act_common_fps_match_window_common_idx != -1))
            applist_idx_list.push_back(act_common_fps_match_window_common_idx);
        if ((act_match_fps_common_window_common_idx != -1))
            applist_idx_list.push_back(act_match_fps_common_window_common_idx);
        if ((act_match_fps_match_window_common_idx != -1))
            applist_idx_list.push_back(act_match_fps_match_window_common_idx);
        if ((act_common_fps_common_window_match_idx != -1))
            applist_idx_list.push_back(act_common_fps_common_window_match_idx);
        if ((act_common_fps_match_window_match_idx != -1))
            applist_idx_list.push_back(act_common_fps_match_window_match_idx);
        if ((act_match_fps_common_window_match_idx != -1))
            applist_idx_list.push_back(act_match_fps_common_window_match_idx);
        if ((act_match_fps_match_window_match_idx != -1))
            applist_idx_list.push_back(act_match_fps_match_window_match_idx);
    }

    return 0;
}

extern "C"
int applistTest(void) {
    int testAppCount = 2, pid = 1234;
    char testPackageName[32] = "com.android.test", testActivityName[32] = "com.android.test";

    LOG_I("enter test mode");

    pthread_mutex_lock(&fg_mutex);
    if(APPList != NULL) {
        free(APPList);
        APPList = NULL;
    }
    if ((APPList = (APPscenario*)malloc(sizeof(APPscenario) * (testAppCount))) == NULL) {
        ALOGE("Can't allocate memory for APPList");
        pthread_mutex_unlock(&fg_mutex);
        return -1;
    }

    _getFPSvalue(&pid, 1);
    set_str_cpy(APPList[0].pack_name, testPackageName, PACK_NAME_MAX);
    set_str_cpy(APPList[0].act_name, testActivityName, CLASS_NAME_MAX);
    APPList[0].pid = pid;
    set_str_cpy(APPList[0].mode, "\0", CLASS_NAME_MAX);
    set_str_cpy(APPList[0].fps, "Common", PACK_NAME_MAX);
    set_str_cpy(APPList[0].window_mode, "Common", CLASS_NAME_MAX);
    APPList[0].list_len = 0;

    set_str_cpy(APPList[1].pack_name, testPackageName, PACK_NAME_MAX);
    set_str_cpy(APPList[1].act_name, "Common", CLASS_NAME_MAX);
    APPList[1].pid = pid;
    set_str_cpy(APPList[1].mode, "\0", CLASS_NAME_MAX);
    set_str_cpy(APPList[1].fps, "Common", PACK_NAME_MAX);
    set_str_cpy(APPList[1].window_mode, "Common", CLASS_NAME_MAX);
    APPList[1].list_len = 0;

    PackCount = testAppCount;

    pthread_mutex_unlock(&fg_mutex);

    return 0;
}

extern "C" 
void setThreadHintControllerEnabled(int enabled)
{
    if (enabled == 1) {
        threadHintControllerEnabled = true;
    } else if (enabled == 0) {
        for (int i = 0; i < ThreadHintPolicyCount; i++) {
            disableThreadHintPolicy(i, ThreadHintCfg[i].pid);
        }

        threadHintControllerEnabled = false;
    }
}

extern "C"
void reloadAppListXml(void) {
    int i, j, k;

    LOG_I("reload applist config");

    pthread_mutex_lock(&fg_mutex);
    for (i = 0; i < PackCount; i ++) {
        if (AppScenarioStatusInfo[i]) {
            LOG_I("Disable scenario when APP(idx=%d)", i);
            AppScenarioDisable(i);
            AppScenarioStatusInfo[i] = 0;
        }
    }

    parseAPPlist();

    if(APPList == NULL) {
        LOG_E("Fail to reload applist config");
        pthread_mutex_unlock(&fg_mutex);
        return;
    }

    applist_idx_list.clear();
    for(i = 0; i < multi_resumed_app_count; i++) {
        findAppScenarioIndex(multi_resumed_app_info[i].packName, multi_resumed_app_info[i].actName,
        APPList, multi_resumed_app_info[i].fps, isMultiWindow, currentMode);
    }

    for (i = 0; i < applist_idx_list.size(); i++) {
        LOG_I("applist_idx_list[%d] %d", i, applist_idx_list[i]);
        LOG_I("AppScenarioStatusInfo[%d] %d", i, AppScenarioStatusInfo[applist_idx_list[i]]);
    }

    for (i = 0; i < PackCount; i++) {
        int enabled = 0, pid = 0;
        for (j = 0; j < applist_idx_list.size(); j++) {
            if (i == applist_idx_list[j])
                enabled = 1;
        }
        if (enabled && (AppScenarioStatusInfo[i] == 0)) {
            for(k = 0; k < multi_resumed_app_count; k++) {
                if (strcmp(multi_resumed_app_info[k].packName, APPList[i].pack_name) == 0) {
                    pid = multi_resumed_app_info[k].pid;
                    break;
                }
            }
            if(pid > 0)
                AppScenarioEnable(i, pid);
            AppScenarioStatusInfo[i] = 1;
        }
    }

    pthread_mutex_unlock(&fg_mutex);
}


extern "C"
int notifyAPPstate(const char *packName, const char *actName, int pid, int activityId, int state, int uid)
{
    int i = 0, j = 0;
    char str_state[STR_LEN_MAX];
    int multiresume_find = 0;
    int foreground_app_count = 0;
    int hdl1 = 0, hdl2 = 0, boost_time = 0;

    if(packName == NULL || actName == NULL) {
        LOG_E("package name or activity name is null!");
        return -1;
    }

    pthread_mutex_lock(&fg_mutex);
    if (state == STATE_PAUSED) {
        strncpy(str_state, "PAUSED", STR_LEN_MAX-1);
    } else if (state == STATE_RESUMED) {
        strncpy(str_state, "RESUMED", STR_LEN_MAX-1);
    } else if (state == STATE_DEAD) {
        strncpy(str_state, "DEAD", STR_LEN_MAX-1);
    } else if (state == FPS_UPDATED) {
        strncpy(str_state, "FPS_UPDATED", STR_LEN_MAX-1);
    } else if (state == MODE_UPDATED) {
        strncpy(str_state, "MODE_UPDATED", STR_LEN_MAX-1);
    }
    LOG_I("%s/%s, pid=%d, activityId=%d, uid=%d, mode:%s state:%s, fps:%d, win:%d", packName, actName, pid, activityId, uid, currentMode, str_state, currentFPS, isMultiWindow);

    fg_launch_time_cold = fg_launch_time_warm = fg_act_switch_time = 0;

    // update foreground app info
    if (state == STATE_RESUMED || state == FPS_UPDATED || state == MULTI_WINDOW) {
        for (i = 0; i< PackCount; i++) {
            if (0 == strcmp(APPList[i].pack_name, packName)) {
                APPList[i].pid = pid;
                LOG_I("PackCount:%d, APPList index:%d, pack:%s, act:%s, state:%s, pid:%d", PackCount, i, packName, actName, str_state, pid);
            }
        }
        for (i = 0; i < ThreadHintPolicyCount; i++) {
            if (0 == strncmp(ThreadHintCfg[i].package, packName, strlen(packName))) {
                ThreadHintCfg[i].pid = pid;
                LOG_D("assign pid=%d to ThreadHintCfg[%d].package=%s", ThreadHintCfg[i].pid, i, ThreadHintCfg[i].package);
            }
        }

        for (i = 0; i < multi_resumed_app_count; i++) {
            if (strcmp(multi_resumed_app_info[i].packName, packName) == 0 && strcmp(multi_resumed_app_info[i].actName, actName) == 0
            && multi_resumed_app_info[i].activityId == activityId) {
                multiresume_find = 1;
                break;
            }
        }
        if (multiresume_find == 0) {
            if (multi_resumed_app_count >= 0 && multi_resumed_app_count < MULTI_WIN_SIZE_MAX) {
                strncpy(multi_resumed_app_info[multi_resumed_app_count].packName, packName, PACK_NAME_MAX-1); // update pack name
                multi_resumed_app_info[multi_resumed_app_count].packName[PACK_NAME_MAX-1] = '\0';
                strncpy(multi_resumed_app_info[multi_resumed_app_count].actName, actName, CLASS_NAME_MAX-1);
                multi_resumed_app_info[multi_resumed_app_count].actName[CLASS_NAME_MAX-1] = '\0';
                multi_resumed_app_info[multi_resumed_app_count].fps = currentFPS;
                multi_resumed_app_info[multi_resumed_app_count].pid = pid;
                multi_resumed_app_info[multi_resumed_app_count].activityId = activityId;
                multi_resumed_app_info[multi_resumed_app_count].uid = uid;
                multi_resumed_app_info[multi_resumed_app_count].is_multi_window = isMultiWindow;
                multi_resumed_app_count++;
            }
        } else if (multiresume_find == 1) { // update fps/window_mode of foreground app
            for (i = 0; i < multi_resumed_app_count; i++) {
                if (strcmp(multi_resumed_app_info[i].packName, packName) == 0 && strcmp(multi_resumed_app_info[i].actName, actName) == 0
                && multi_resumed_app_info[i].activityId == activityId) {
                    multi_resumed_app_info[i].fps = currentFPS;
                    multi_resumed_app_info[i].is_multi_window = isMultiWindow;
                }
            }
        }

    } else if (state == STATE_PAUSED) {  // remove resumed list
        if(multi_resumed_app_count == 1 && strcmp(multi_resumed_app_info[0].packName, packName) == 0 && strcmp(multi_resumed_app_info[0].actName, actName) == 0
        && multi_resumed_app_info[0].pid == pid && multi_resumed_app_info[0].activityId == activityId) {
            strncpy(multi_resumed_app_info[0].packName, "\0", PACK_NAME_MAX-1);
            strncpy(multi_resumed_app_info[0].actName, "\0", CLASS_NAME_MAX-1);
            multi_resumed_app_info[0].fps = -1;
            multi_resumed_app_info[0].pid = -1;
            multi_resumed_app_info[0].activityId = -1;
            multi_resumed_app_info[0].uid = -1;
            multi_resumed_app_info[0].is_multi_window = -1;
            multi_resumed_app_count--;
        } else {
            for (i = 0; i < multi_resumed_app_count; i++) {
                if (strcmp(multi_resumed_app_info[i].packName, packName) == 0 && strcmp(multi_resumed_app_info[i].actName, actName) == 0
                && multi_resumed_app_info[i].pid == pid && multi_resumed_app_info[i].activityId == activityId) {
                    for ( j = i; j < (multi_resumed_app_count-1); j++) {
                        strncpy(multi_resumed_app_info[j].packName, multi_resumed_app_info[j+1].packName, PACK_NAME_MAX-1);
                        multi_resumed_app_info[j].packName[PACK_NAME_MAX-1] = '\0';
                        strncpy(multi_resumed_app_info[j].actName, multi_resumed_app_info[j+1].actName, CLASS_NAME_MAX-1);
                        multi_resumed_app_info[j].actName[CLASS_NAME_MAX-1] = '\0';
                        multi_resumed_app_info[j].fps = multi_resumed_app_info[j+1].fps;
                        multi_resumed_app_info[j].pid = multi_resumed_app_info[j+1].pid;
                        multi_resumed_app_info[j].activityId = multi_resumed_app_info[j+1].activityId;
                        multi_resumed_app_info[j].uid = multi_resumed_app_info[j+1].uid;
                        multi_resumed_app_info[j].is_multi_window = multi_resumed_app_info[j+1].is_multi_window;
                    }
                    multi_resumed_app_count--;
                    break;
                }
            }
        }
    } else if (state == STATE_DEAD) {
        if(multi_resumed_app_count == 1 && multi_resumed_app_info[0].pid == pid) {
            strncpy(multi_resumed_app_info[0].packName, "\0", PACK_NAME_MAX-1);
            strncpy(multi_resumed_app_info[0].actName, "\0", CLASS_NAME_MAX-1);
            multi_resumed_app_info[0].fps = -1;
            multi_resumed_app_info[0].pid = -1;
            multi_resumed_app_info[0].activityId = -1;
            multi_resumed_app_info[0].uid = -1;
            multi_resumed_app_info[0].is_multi_window = -1;
            multi_resumed_app_count--;
        } else {
            for (i = 0; i < multi_resumed_app_count; i++) {
                if (multi_resumed_app_info[i].pid == pid) {
                    for (j = i; j < (multi_resumed_app_count-1); j++) {
                        strncpy(multi_resumed_app_info[j].packName, multi_resumed_app_info[j+1].packName, PACK_NAME_MAX-1);
                        multi_resumed_app_info[j].packName[PACK_NAME_MAX-1] = '\0';
                        strncpy(multi_resumed_app_info[j].actName, multi_resumed_app_info[j+1].actName, CLASS_NAME_MAX-1);
                        multi_resumed_app_info[j].actName[CLASS_NAME_MAX-1] = '\0';
                        multi_resumed_app_info[j].fps = multi_resumed_app_info[j+1].fps;
                        multi_resumed_app_info[j].pid = multi_resumed_app_info[j+1].pid;
                        multi_resumed_app_info[j].activityId = multi_resumed_app_info[j+1].activityId;
                        multi_resumed_app_info[j].uid = multi_resumed_app_info[j+1].uid;
                        multi_resumed_app_info[j].is_multi_window = multi_resumed_app_info[j+1].is_multi_window;
                    }
                    i--;
                    multi_resumed_app_count--;
                }
            }
        }
    }

    LOG_I("multi_resumed_app_count: %d", multi_resumed_app_count);
    for (i = 0; i < multi_resumed_app_count; i++)
       LOG_I("multi_resumed_app_info[%d] %s/%s, pid:%d, activityId:%d, fps:%d, isMultiWindow:%d",
       i, multi_resumed_app_info[i].packName,multi_resumed_app_info[i].actName, multi_resumed_app_info[i].pid,
       multi_resumed_app_info[i].activityId, multi_resumed_app_info[i].fps, multi_resumed_app_info[i].is_multi_window);

    foreground_app_count = getForegroundAPPCount();
    if (foreground_app_count > 1 && isMultiWindow == 0) {
        isMultiWindow = 1;
    } else if (foreground_app_count <= 1 && isMultiWindow == 1) {
        isMultiWindow = 0;
    }

    if (state == STATE_DEAD) {
        LOG_I("pack:%s, pid:%d, STATE_DEAD", packName, pid);

        for (i = 0; i< PackCount; i++) {
            if (pid == APPList[i].pid && pid != 0) {
                LOG_I("PackCount:%d, pack:%s, com:%s, state:DEAD, pid:%d", PackCount, packName, actName, pid);
                AppScenarioDisable(i);
            }
        }

        for (i = 0; i< ThreadHintPolicyCount; i++) {
            if (pid == ThreadHintCfg[i].pid && pid != 0) {
                LOG_I("ThreadHintPolicyCount=%d, pack=%s, pid=%d, state=DEAD", ThreadHintPolicyCount, packName, pid);
                disableThreadHintPolicy(i, ThreadHintCfg[i].pid);
            }
        }

        for (i = 0; i < multi_resumed_app_count; i ++) {
            if (pid == multi_resumed_app_info[i].pid) {
                for (j = 0; j < 0 + PackCount; j ++) {
                    if (0 == strcmp(APPList[j].pack_name, packName)) {
                        LOG_I("Disable scenario when APP(pid=%d) crash", pid);
                        AppScenarioDisable(j);
                    }
                }

                for (j = 0; j < ThreadHintPolicyCount; j ++) {
                    if (0 == strncmp(ThreadHintCfg[j].package, packName, strlen(packName))) {
                        LOG_I("Disable scenario when APP(pid=%d) crash", pid);
                        disableThreadHintPolicy(j, ThreadHintCfg[j].pid);
                    }
                }
            }
        }
    }

    /* foreground change: update pack name */
    if(state == STATE_RESUMED || state == FPS_UPDATED) {
        strncpy(foreground_app_info.packName, packName, PACK_NAME_MAX-1);
        strncpy(foreground_app_info.actName, actName, PACK_NAME_MAX-1);
        foreground_app_info.pid = pid;
        foreground_app_info.activityId = activityId;
        foreground_app_info.uid = uid;

        if (state != FPS_UPDATED)
            LOG_I("foreground:%s, pid:%d, activityId:%d, uid:%d", foreground_app_info.packName, pid, activityId, uid);
    }

    notifyNetdUID(foreground_app_info.uid);
    notifyPID(foreground_app_info.pid);



    // handle applist scenarios ...
    applist_idx_list.clear();
    for(i = 0; i < multi_resumed_app_count; i++) {
        findAppScenarioIndex(multi_resumed_app_info[i].packName, multi_resumed_app_info[i].actName,
                            APPList, multi_resumed_app_info[i].fps, isMultiWindow, currentMode);
    }

    for (i = 0; i < applist_idx_list.size(); i++) {
        LOG_I("applist_idx_list[%d] %d", i, applist_idx_list[i]);
        LOG_I("AppScenarioStatusInfo[%d] %d", i, AppScenarioStatusInfo[applist_idx_list[i]]);
    }

    for (i = 0; i < PackCount; i++) {
        int enabled = 0;

        for (j = 0; j < applist_idx_list.size(); j++) {
            if (i == applist_idx_list[j])
                enabled = 1;
        }
        if ((!enabled) && (AppScenarioStatusInfo[i] == 1)) {
            AppScenarioDisable(i);
            AppScenarioStatusInfo[i] = 0;
        }
    }
    for (i = 0; i < PackCount; i++) {
        int enabled = 0, acv_pid = pid;
        for(j = 0; j < multi_resumed_app_count; j++) {
            if (0 == strcmp(APPList[i].pack_name, multi_resumed_app_info[j].packName))
                acv_pid = multi_resumed_app_info[j].pid;
        }
        for (j = 0; j < applist_idx_list.size(); j++) {
            if (i == applist_idx_list[j])
                enabled = 1;
        }
        if (enabled && (AppScenarioStatusInfo[i] == 0)) {
            AppScenarioEnable(i, acv_pid);
            AppScenarioStatusInfo[i] = 1;
        }
    }



    // handle ThreadHint scenarios ...
    currentCandidateThreadHintPolicyIdx.clear();
    currentCandidateThreadHintPolicy.clear();

    for (i = 0; i < multi_resumed_app_count; i++) {
        findThreadHintPolicyIdx(multi_resumed_app_info[i].packName, ThreadHintCfg, multi_resumed_app_info[i].fps, isMultiWindow);
    }

    LOG_D("currentCandidateThreadHintPolicyIdx.size()=%zu", currentCandidateThreadHintPolicyIdx.size());

    for (i = 0; i < currentCandidateThreadHintPolicyIdx.size(); i++) {
        currentCandidateThreadHintPolicy.push_back(ThreadHintCfg[currentCandidateThreadHintPolicyIdx[i]]);
    }

    LOG_D("currentCandidateThreadHintPolicy.size()=%zu", currentCandidateThreadHintPolicy.size());

    for (i = 0; i < currentCandidateThreadHintPolicy.size(); i++) {
        LOG_D("currentCandidateThreadHintPolicy[%d] package=%s, fps=%s, window=%s, thread=%s",
            i, currentCandidateThreadHintPolicy[i].package, currentCandidateThreadHintPolicy[i].fps, currentCandidateThreadHintPolicy[i].window, currentCandidateThreadHintPolicy[i].thread);
    }


    for (i = 0; i < ThreadHintPolicyCount; i++) {
        bool enabled = false;

        for (j = 0; j < currentCandidateThreadHintPolicyIdx.size(); j++) {
            if (i == currentCandidateThreadHintPolicyIdx[j]) {
                enabled = true;
                break;
            }
        }

        if (!enabled && (isThreadHintPolicyEnabled[i] == 1)) {
            disableThreadHintPolicy(i, ThreadHintCfg[i].pid);
            isThreadHintPolicyEnabled[i] = 0;
        }

        if (enabled && (isThreadHintPolicyEnabled[i] == 0)) {
            isThreadHintPolicyEnabled[i] = 1;
        }
    }

    unsigned int finalThreadHintPolicyCount = decideFinalThreadHintPolicy(currentCandidateThreadHintPolicy);
    LOG_D("final ThreadHint policy count = %u", finalThreadHintPolicyCount);




    /* package switch => check white list boost */
    if (state == STATE_RESUMED) {
        if (strcmp(packName, currPackname) != 0) {
            LOG_I("pc:%d, %s => %s", bDuringProcessCreate, currPackname, packName);
            strncpy(currPackname, packName, STR_LEN_MAX-1);

            hdl1 = hdl_launch;

            bDuringProcessCreate = querySysInfo(MTKPOWER_CMD_GET_PROCESS_CREATE_STATUS, 0);
            if (bDuringProcessCreate) {
                boost_time = fg_launch_time_cold;
            } else {
                boost_time = fg_launch_time_warm;
            }

            if (boost_time > 0) {
                hdl2 = perfCusLockHint(MTKPOWER_HINT_WHITELIST_LAUNCH, boost_time);
                LOG_I("%s boost_time=%d", currPackname, boost_time);
            }

            if (hdl1 > 0) {
                perfLockRel(hdl1);
            }
            hdl_launch = hdl2;

        } else if (strcmp(actName, currActname) != 0) {
            LOG_I("act %s => %s", currActname, actName);
            strncpy(currActname, actName, sizeof(currActname)-1);

            hdl1 = hdl_act_switch;
            boost_time = fg_act_switch_time;

            if (boost_time > 0) {
                hdl2 = perfCusLockHint(MTKPOWER_HINT_WHITELIST_ACT_SWITCH, boost_time);
                LOG_I("%s boost_time=%d", currActname, boost_time);
            }

            if (hdl1 > 0) {
                perfLockRel(hdl1);
            }
            hdl_act_switch = hdl2;
        }
    }
    pthread_mutex_unlock(&fg_mutex);


    return 0;
}




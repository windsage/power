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

#define STR_LEN_MAX 256
#define MAX_ARGS_PER_REQUEST 100
#define LOG_TAG "MTK_APPList"
#define PACK_NAME_MAX 128
#define CLASS_NAME_MAX 128
#define COMM_NAME_SIZE  64
#define MULTI_WIN_SIZE_MAX 30
#define FBC_LIB_FULL_NAME "libperfctl_vendor.so"
#define APP_LIST_XMLPATH "/vendor/etc/power_app_cfg.xml"
#define APP_LIST_LOOM_XMLPATH "/vendor/etc/power_app_cfg_loom.xml"
#define THREAD_HINT_CFG_XML_PATH "/vendor/etc/loom.cfg.xml"
#define SYSFS_GAME_LOOM_TASK_CFG_PATH "/sys/kernel/game/loom_task_cfg"
#define LOOM_CMD_DEFAULT_VALUE -1
#define FPS_SPECIFIED 2
#define FPS_COMMON 1
#define WINDOW_SPECIFIED 2
#define WINDOW_COMMON 1
#define CMD_CSV_PATH "/vendor/etc/command.csv"
#define FPS_TOLERANCE_PERCENT 10
#define PERF_LOCK_LIB_FULL_NAME  "libmtkperf_client_vendor.so"
#define POWERHAL_WRAP_FULL_NAME  "libpowerhalwrap_vendor.so"

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)


struct _activity {
    char actName[128];
    char packName[128];
    int is_multi_window;
    int fps;
    int pid;
    int uid;
    int onTop;
    int activityId;
};

struct xml_element {
    char cmd[128];
    char actName[128];
    char packName[128];
    char sbefeatureName[128];
    char fps[128];
    char window_mode[128];
    char threadName[128];
    char mode[128];
    int param1;
    int pid;
    int uid;
};

enum {
    SETSYS_MANAGEMENT_PREDICT            = 1,
    SETSYS_SPORTS_APK                    = 2,
    SETSYS_FOREGROUND_SPORTS             = 3,
    SETSYS_MANAGEMENT_PERIODIC           = 4,
    SETSYS_INTERNET_STATUS               = 5,
    SETSYS_NETD_STATUS                   = 6,
    SETSYS_PREDICT_INFO                  = 7,
    SETSYS_NETD_DUPLICATE_PACKET_LINK    = 8,
    SETSYS_PACKAGE_VERSION_NAME          = 9,
    SETSYS_RELOAD_WHITELIST              = 10,
    SETSYS_POWERHAL_UNIT_TEST            = 11,
    SETSYS_API_ENABLED                   = 12,
    SETSYS_API_DISABLED                  = 13,
    SETSYS_FPS_VALUE                     = 14,
    SETSYS_NETD_SET_FASTPATH_BY_UID      = 15,
    SETSYS_NETD_SET_FASTPATH_BY_LINKINFO = 16,
    SETSYS_NETD_CLEAR_FASTPATH_RULES     = 17,
    SETSYS_NETD_BOOSTER_CONFIG           = 18,
    SETSYS_MULTI_WINDOW_STATUS           = 19,
    SETSYS_FPSGO_GAME_MODE_PID           = 20,
    SETSYS_FPSGO_VIDEO_MODE_PID          = 21,
    SETSYS_NETD_SET_BOOST_UID            = 22,
    SETSYS_POWERHAL_GAME_MODE_ENABLED    = 23,
    SETSYS_FOREGROUND_APP_PID            = 24,
};

typedef struct APPscenario{
    int handle_idx;
    int pid;
    int tid;
    int uid;
    char pack_name[STR_LEN_MAX];
    char act_name[STR_LEN_MAX];
    char mode[STR_LEN_MAX];
    char sbe_featurename[STR_LEN_MAX];
    char fps[STR_LEN_MAX];
    char window_mode[STR_LEN_MAX];
    int lock_rsc_list[MAX_ARGS_PER_REQUEST];
    int list_len;
}APPscenario;



// ThreadHint
enum {
    MODE = 0,
    MATCHING_NUM,
    PRIO,
    CPU_MASK,
    SET_EXCLUSIVE,
    LOADING_UB,
    LOADING_LB,
    BHR,
    LIMIT_MIN_FREQ,
    LIMIT_MAX_FREQ,
    SET_RESCUE,
    RESCUE_F_OPP,
    RESCUE_C_FREQ,
    RESCUE_TIME,
    THREAD_HINT_PARAM_NUM,
};

struct ThreadHintXmlElement {
    int pid;
    int uid;
    char package[STR_LEN_MAX];
    char fps[STR_LEN_MAX];
    char window[STR_LEN_MAX];
    char thread[STR_LEN_MAX];
    char cmd[STR_LEN_MAX];
    int param1;
};

typedef struct ThreadHintPolicy {
    int pid;
    int uid;
    char package[STR_LEN_MAX];
    char fps[STR_LEN_MAX];
    char window[STR_LEN_MAX];
    char thread[STR_LEN_MAX];
    int cmd[MAX_ARGS_PER_REQUEST];
    int cmd_size;
//
    int mode;           int mode_valid;
    int matching_num;   int matching_num_valid;
    int prio;           int prio_valid;
    int cpu_mask;       int cpu_mask_valid;
    int set_exclusive;  int set_exclusive_valid;
    int loading_ub;     int loading_ub_valid;
    int loading_lb;     int loading_lb_valid;
    int bhr;            int bhr_valid;
    int limit_min_freq; int limit_min_freq_valid;
    int limit_max_freq; int limit_max_freq_valid;
    int set_rescue;     int set_rescue_valid;
    int rescue_f_opp;   int rescue_f_opp_valid;
    int rescue_c_freq;  int rescue_c_freq_valid;
    int rescue_time;    int rescue_time_valid;
} ThreadHintPolicy;

typedef struct ThreadHintParams {
    int cmdId;
    char cmdString[STR_LEN_MAX];
} ThreadHintParams;

ThreadHintParams ThreadHintParamsMappingTbl[] = {
    {
        .cmdId = MODE,
        .cmdString = "MODE",
    },
    {
        .cmdId = MATCHING_NUM,
        .cmdString = "MATCHING_NUM",
    },
    {
        .cmdId = PRIO,
        .cmdString = "PRIO",
    },
    {
        .cmdId = CPU_MASK,
        .cmdString = "CPU_MASK",
    },
    {
        .cmdId = SET_EXCLUSIVE,
        .cmdString = "SET_EXCLUSIVE",
    },
    {
        .cmdId = LOADING_UB,
        .cmdString = "LOADING_UB",
    },
    {
        .cmdId = LOADING_LB,
        .cmdString = "LOADING_LB",
    },
    {
        .cmdId = BHR,
        .cmdString = "BHR",
    },
    {
        .cmdId = LIMIT_MIN_FREQ,
        .cmdString = "LIMIT_MIN_FREQ",
    },
    {
        .cmdId = LIMIT_MAX_FREQ,
        .cmdString = "LIMIT_MAX_FREQ",
    },
    {
        .cmdId = SET_RESCUE,
        .cmdString = "SET_RESCUE",
    },
    {
        .cmdId = RESCUE_F_OPP,
        .cmdString = "RESCUE_F_OPP",
    },
    {
        .cmdId = RESCUE_C_FREQ,
        .cmdString = "RESCUE_C_FREQ",
    },
    {
        .cmdId = RESCUE_TIME,
        .cmdString = "RESCUE_TIME",
    },
};




extern "C" int AppScenarioEnable(int idx, int pid);
extern "C" int AppScenarioDisable(int idx);
extern "C" int findAppScenarioIndex(const char *packName, const char *actName, APPscenario *APPList, int mycurrentFPS);
extern "C" _activity* getForegroundAPPInfo();
extern "C" int _checkFPSupdate(_activity* fg);
extern "C" int notifyAPPstate(const char *packName, const char *actName, int pid, int activityId, int state, int uid);
extern "C" int parseAPPlist(void);
extern "C" int setCurrentMode(const char *mode);
extern "C" int getCurrentMode(char *curMode);
extern "C" int setAPPListConfig(int configIdx);
extern "C" void reloadAppListXml(void);
extern "C" void setThreadHintControllerEnabled(int enabled);
extern "C" int applistTest(void);
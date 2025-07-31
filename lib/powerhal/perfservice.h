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

#ifndef ANDROID_PERFSERVICE_H
#define ANDROID_PERFSERVICE_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>
#include <utils/threads.h>
#include "perfservice_types.h"

//#define PROJ_ALL "all"

using namespace android;
using namespace std;

#define CFG_TBL_INVALID_VALUE (-123456)

#ifndef LOG_TAG
#define LOG_TAG "libPowerHal"
#endif

#define LOG_E(fmt, arg...)  ALOGE("[%s] " fmt, __func__, ##arg)
#define LOG_I(fmt, arg...)  ALOGI("[%s] " fmt, __func__, ##arg)
#define LOG_D(fmt, arg...)  ALOGD("[%s] " fmt, __func__, ##arg)
#define LOG_V(fmt, arg...)  ALOGV("[%s] " fmt, __func__, ##arg)

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
    SETSYS_FOREGROUND_APP_PID = 24,
    SETSYS_CAMERA_MODE_PID      = 25,
};

enum {
    NORMAL_MODE = 0,
    GAME_MODE,
    MODE_COUNT,
};

enum {
    STATUS_PERFLOCKACQ_ENABLED = 0,
    STATUS_CUSLOCKHINT_ENABLED,
    STATUS_APPLIST_ENABLED,
    STATUS_API_COUNT,
};

enum {
    DUMP_DEFAULT     = 0,
    DUMP_CPU_CTL     = 1,
    DUMP_GPU_CTL     = 2,
    DUMP_CMD_CTL     = 3,
    DUMP_ALL_USER    = 4,
    DUMP_APP_CFG     = 5,
};

struct xml_activity {
    char cmd[128];
    char actName[128];
    char packName[128];
    char sbefeatureName[128];
    char fps[128];
    char window_mode[128];
    int param1;
    int param2;
    int param3;
    int param4;
    int delete_flag;
    int pid;
    int uid;
};

struct resumed_activity {
    char actName[128];
    char packName[128];
    int is_multi_window;
    int fps;
    int pid;
    int uid;
    int onTop;
};

typedef struct tScnConTable {
    string  cmdName;
    int     cmdID;
    int     legacyCmdID;
    string  entry;
    int     defaultVal;
    int     curVal;
    string  comp;
    int     maxVal;
    int     minVal;
    int     resetVal; // value to reset this entry
    int     isValid;
    int     normalVal;
    int     gameVal;
    int     sportVal;
    int     ignore;
    int     init_set_default;
    string  prefix;
} tScnConTable;

enum {
    ONESHOT       = 0,
    BIGGEST       = 1,
    SMALLEST      = 2,
};

typedef int (*rsc_set)(int, void*);
typedef int (*rsc_unset)(int, void*);
typedef int (*rsc_init)(int);

typedef struct tRscConfig {
    int     cmdID; // -1 : final entry
    string  cmdName;
    int     comp;
    int     maxVal;
    int     minVal;
    int     defaultVal;
    int     normalVal;
    int     gameVal;
    int     sportVal;
    int     force_update; // always trigger update lock
    rsc_set   set_func;
    rsc_unset unset_func;
    rsc_init  init_func;
} tRscConfig;

typedef struct tRscCtlEntry {
    int     curVal;
    int     log;
    int     isValid;
    int     resetVal; // value to reset this entry
} tRscCtlEntry;

typedef struct _cpufreq {
    int min;
    int max;
} _cpufreq;


enum {
    SCN_POWER_HINT      = 0,
    SCN_CUS_POWER_HINT  = 1,
    SCN_USER_HINT       = 2,
    SCN_PERF_LOCK_HINT  = 3,
    SCN_PACK_HINT       = 4,
};

int init();
int isPowerhalReady();
int isPowerLibEnable();
void resetScenario(int handle, int reset_all);
int perfScnDisable(int scenario);
int perfScnEnable(int scenario);

void switchNormalAndSportMode(int mode);
int setGameMode(int enable, void *scn);
int disableGameMode(int enable, void *scn);
int cmdSetting(int icmd, char *scmd, tScnNode *scenario, int param_1, int *rsc_id);
void Scn_cmdSetting(char *cmd, int scn, int param_1);
int getForegroundInfo(char **pPackName, int *pPid, int *pUid);
int setAPIstatus(int cmd, int enable);
int queryInteractiveState();
int queryAPIstatus(int cmd);
xml_activity* get_foreground_app_info();

/* extern api */
int perfNotifyAppState(const char *packName, const char *actName, int state, int pid, int uid);
int perfSetSysInfo(int type, const char *data);
int perfUserGetCapability(int cmd, int id);
int perfUserScnDisableAll(void);
int perfUserScnRestoreAll(void);
int perfUserScnCheckAll(void);
int perfDumpAll(int cmd);

int perfLibpowerInit(void);
int perfLockCheckHdl(int handle, int hint, int size, int pid, int tid, int duration);
int perfLockAcq(int *list, int handle, int size, int pid, int tid, int duration, int hint);
int perfLockRel(int handle);
int perfGetHintRsc(int hint, int *size, int *rsc_list);
int perfCusLockHint(int hint, int duration, int pid);

/* smart detect */
int smart_init(void);
int smart_reset(const char *pack, int pid);
int smart_check_pack_existed(int type, const char *pack, int uid, int fromUid);
int smart_set_benchmark_mode(int benchmark);
int smart_add_pack(const char *pack);
int smart_add_benchmark(void);
int smart_add_specific_benchmark(const char *pack);

int loadConTable(const char *file_name);
void disableScenarioByHintId(int hindId);
int getCPUFreq(int cid, int opp);
int get_cpu_dma_latency_value();
int get_system_mask_status(int cmd);
int set_applist_fps_value(int value, void *scn);
//SPD: add powerhal reinit by sifengtian 20230711 start
int reinit(int);
//SPD: add powerhal reinit by sifengtian 20230711 end
//SPD:thermal ux modify for k510 by sifengtian 20230721 start
int getPpmSupport();
//SPD:thermal ux modify for k510 by sifengtian 20230721 end
void processMBrain();
#endif // ANDROID_PERFSERVICE_H


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

#define LOG_TAG "libPowerHal"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#define LOG_NDEBUG 0 // support ALOGV

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
#include <utils/Trace.h>
#include "perfservice.h"
#include "perfservice_xmlparse.h"
#include "common.h"
#include "tran_common.h"

#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <expat.h>
#include <time.h>
#include <mutex>
#include <chrono>
#include <thread>

#include "mtkpower_hint.h"
#include "mtkperf_resource.h"
#include "mtkpower_types.h"
#include <utils/Timers.h>
#include "perfservice_rsccfgtbl.h"
#include "perfservice_prop.h"
#include "utility_ux.h"
#include "utility_ril.h"
#include "utility_adpf.h"
#include "utility_netd.h"
#include "utility_touch.h"
#include "power_msg_handler.h"
#include "power_boot_handler.h"
#include "power_error_handler.h"
#include "perfservice_scn.h"
#include "utility_sbe_handle.h"
#include "utility_fps.h"
#include "utility_sched.h"

#if 1 //HAVE_MBRAIN
typedef int (*NotifyCpuFreqCapSetupHook)(int pid, int tid, int hdl, int duration, int hintId, int clusterId, int qosType, int freq);
typedef int (*NotifyGameModeEnabledHook)(bool enabled);

#if defined(_LP64)
const char g_libMBSDKvFilename[] = "/vendor/lib64/libmbrainSDKv.so";
#else // _LP64
const char g_libMBSDKvFilename[] = "/vendor/lib/libmbrainSDKv.so";
#endif // _LP64
#endif

#ifdef HAVE_AEE_FEATURE
#include "aee.h"
#endif

#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPower.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPowerCallback.h>
using namespace vendor::mediatek::hardware::mtkpower::V1_2;
using ::vendor::mediatek::hardware::mtkpower::V1_2::IMtkPowerCallback;
using ::android::sp;
using android::hardware::Return;

//#include "tinyxml2.h"
//using namespace tinyxml2;

/* Definition */
#define VERSION "11.0"

#define STATE_ON 1
#define STATE_OFF 0
#define STATE_WAIT_RESTORE 2

#define CLASS_NAME_MAX  128
#define CUS_SCN_TABLE           "/vendor/etc/powerscntbl.xml"
#define CUS_CONFIG_TABLE        "/vendor/etc/powercontable.xml"
#define CUS_CONFIG_TABLE_T      "/vendor/etc/powercontable_t.xml"
#define POWER_DL_XML_DATA       "/data/vendor/powerhal/powerdlttable.data"
#define COMM_NAME_SIZE  64
#define MULTI_WIN_SIZE_MAX   30

#define REG_SCN_MAX     256   // user scenario max number
#define CLUSTER_MAX     8
#define CPU_CORE_MAX    8
#define RSC_IDX_DIFF    (0x100)

#define FBC_LIB_FULL_NAME  "libperfctl_vendor.so"
#define LIB_POWER_APPLIST "lib_power_applist.so"

#define CPU_CORE_MIN_RESET  (-1)
#define CPU_CORE_MAX_RESET  (0xff)
#define CPU_FREQ_MIN_RESET  (-1)
#define CPU_FREQ_MAX_RESET  (0xffffff)
#define GPU_FREQ_MIN_RESET  (-1)

#define CORE_MAX        0xff
#define FREQ_MAX        0xffffff

// core_ctl default setting
#define CORE_CTL_CPUCORE_MIN_CLUSTER_0 4
#define CORE_CTL_CPUCORE_MIN_CLUSTER_1 2
#define CORE_CTL_CPUCORE_MIN_CLUSTER_2 0

#define HARDLIMIT_ENABLED 1

const string LESS("less");
const string MORE("more");

#define HANDLE_RAND_MAX 2147483645
#define USER_DURATION_MAX 30000 /*30s for non permission user*/

#define RSC_TBL_INVALID_VALUE (-123456)

#define ATRACE_MESSAGE_LEN 256

#ifdef max
#undef max
#endif
#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a,b) (((a) < (b)) ? (a) : (b))

using namespace std;

typedef int (*fbc_get_fstb_active)(long long);
typedef int (*fbc_wait_fstb_active)(void);
typedef int (*fbc_notify_foreground_app)(resumed_activity *, int, const char *, const char *, int, int, int, int);

/* Function prototype */
void setClusterCores(int clusterNum, int totalCore, int *pCoreTbl, int *pMaxCoreTbl);
void recordCPUFreqInfo(int scenario);
void updateCPUFreqInfo(int cluster, int cpu);
void updateClusterFreqInfo(int cluster);
void updateCPUSettingInfo(int cluster, int cpu, int freqToSet, int curMinFreqSetter, string curMinFreqSetterName, 
    int curMinFreqSetterHint, int maxToSet, int curMaxFreqSetter, string curMaxFreqSetterName, int curMaxFreqSetterHint);
void setCPUFreq(int cluster, int cpu);
void setClusterFreq(int cluster);
void setClusterHardFreq(int cluster);
int perfScnEnable(int scenario);
int perfScnDisable(int scenario);
int perfScnUpdate(int scenario, int force_update);
void setGpuFreq(const char * scenario, int level);
void setGpuFreqMax(const char * scenario, int level);
void resetScenario(int handle, int reset_all);
void checkDrvSupport(tDrvInfo *ptDrvInfo);
int getCputopoInfo(int, int *, int *, tClusterInfo **, tCoreInfo **);
int cmdSetting(int, char *, tScnNode *, int, int*);
int loadRscTable(int power_on_init);
static int check_vendor_partition(void);
int testRscTable(void);
int check_core_ctl_ioctl(void);
int getCPUFreq(int cid, int opp);
void update_default_core_min();
int load_mbrain_api(void);
static void notifyCap(int scenario, string type);
void initCPUFreqInfo();

/* Global variable */
static int nIsReady = 0;
static int nPowerLibEnable = 1;
/*static char* sProjName = (char*)PROJ_ALL;*/
static std::mutex sMutex;

int nClusterNum = 0;
int nCpuNum = 0;

static tScnNode  *ptScnList = NULL;

tDrvInfo   gtDrvInfo;
static int        nPackNum = 0;
static int        nUserScnBase = 0;
static int        SCN_APP_RUN_BASE = REG_SCN_MAX;
static int user_handle_now = 0;

//static int last_from_uid  = 1;
static int fg_launch_time_cold = 0;
static int fg_launch_time_warm = 0;
static int fg_act_switch_time = 0;
static int install_max_duration = 0;
static char foreground_pack[PACK_NAME_MAX];
static char foreground_act[PACK_NAME_MAX];
int CpuFreqReady = 0;
static int game_mode_pid = -1;
static int notifyGbePid = 1;
static int video_mode_pid = -1;
static int camera_mode_pid = -1;
xml_activity foreground_app_info;
static int powerhalAPI_status[STATUS_API_COUNT];
int mode_status[MODE_COUNT] = {1, 0};
static int mInteractive = 0;
static int mInteractiveControl = 1;
static int core_ctl_dev_fd = -1;

//Mbrain
int isMbrainSupport = 0;
NotifyCpuFreqCapSetupHook notifyMbrainCpuFreqCap = nullptr;
NotifyGameModeEnabledHook notifyMBrainGameModeEnabled = nullptr;
void* libMBrainHandle = nullptr;

int testSetting = 0;

static int scnCheckSupport = 1;

static nsecs_t last_touch_time = 0;
static nsecs_t last_aee_time = 0;

extern tScnConTable tConTable[FIELD_SIZE];
extern tRscConfig RscCfgTbl[];
tClusterInfo *ptClusterTbl;
tCoreInfo *ptCoreTbl;
int cpufreqNeedUpdate, cpufreqHardNeedUpdate;

int    gRscCtlTblLen = 0;
static tRscCtlEntry *gRscCtlTbl = NULL;

// value of softlimit, hardlimit
_cpufreq soft_freq[CLUSTER_MAX];
_cpufreq hard_freq[CLUSTER_MAX];
_cpufreq soft_core_freq[CPU_CORE_MAX];
_cpufreq freq_to_set[CPU_CORE_MAX];

int nGpuFreqCount;

// core_ctl
int default_core_min[CLUSTER_MAX] = {CORE_CTL_CPUCORE_MIN_CLUSTER_0, CORE_CTL_CPUCORE_MIN_CLUSTER_1, CORE_CTL_CPUCORE_MIN_CLUSTER_2};

int atrace_marker_fd = -1;

//SPD: add powermode by jiacheng.deng 20250512 start
int g_tran_power_mode_support = 0;
#define TRAN_POWER_MODE_SUPPORT "ro.odm.tr_powerhal.power_mode.feature.support"
#define TRAN_CPU_LIMIT_MODE "persist.odm.tr_powerhal.lmode.model"
_cpufreq tran_hard_freq[CLUSTER_MAX];
//SPD: add powermode by jiacheng.deng 20250512 start

int trace_init(void)
{
    atrace_marker_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    if (atrace_marker_fd != -1)
        return 0;
    else
        return -1;
}

void trace_exit(void) {
    close(atrace_marker_fd);
}

inline void trace_count(const char *name, const int pid, const int fpsvalue)
{
    char buf[ATRACE_MESSAGE_LEN];
    int len = snprintf(buf, ATRACE_MESSAGE_LEN, "C|%d|%s|%d", pid, name, fpsvalue);
    if(len < 0) {
        ALOGE("snprint error");
        return;
    }

    write(atrace_marker_fd, buf, len);
}

void trace_cpu_freq(int cluster, int minFreq, int maxFreq, int minSetter, int maxSetter, int minSetterHint, int maxSetterHint)
{
    if(ATRACE_ENABLED()) {
        char buf_min[ATRACE_MESSAGE_LEN], buf_min_setter[ATRACE_MESSAGE_LEN],
            buf_max[ATRACE_MESSAGE_LEN], buf_max_setter[ATRACE_MESSAGE_LEN],
            buf_min_hint[ATRACE_MESSAGE_LEN], buf_max_hint[ATRACE_MESSAGE_LEN];
        if(snprintf(buf_min, ATRACE_MESSAGE_LEN, "powerhal_c%d_min", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        if(snprintf(buf_max, ATRACE_MESSAGE_LEN, "powerhal_c%d_max", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        if(snprintf(buf_min_setter, ATRACE_MESSAGE_LEN, "powerhal_c%d_min_setter", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        if(snprintf(buf_max_setter, ATRACE_MESSAGE_LEN, "powerhal_c%d_max_setter", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        if(snprintf(buf_min_hint, ATRACE_MESSAGE_LEN, "powerhal_c%d_min_hint", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        if(snprintf(buf_max_hint, ATRACE_MESSAGE_LEN, "powerhal_c%d_max_hint", cluster) < 0) {
            LOG_E("sprintf error");
            return;
        }
        ATRACE_INT(buf_min, minFreq);
        ATRACE_INT(buf_min_setter, minSetter);
        ATRACE_INT(buf_max, maxFreq);
        ATRACE_INT(buf_max_setter, maxSetter);
        ATRACE_INT(buf_min_hint, minSetterHint);
        ATRACE_INT(buf_max_hint, maxSetterHint);
    }
}

int getClusterNum() {
    return nClusterNum ? nClusterNum : -1;
}

//SPD: add powerhal reinit by sifengtian 20230711 start
int reinit(int reinit_version)
{
    int i;
    const char * scn_file_path;

    /* V.10: only support thermal_ux*/
    ALOGI("[reinit] version:%d", reinit_version);

    if (reinit_version == 1) {
        //thermalUxPolicyReInit();
    }
    ALOGI("[reinit] end");
    return 0;
}
//SPD: add powerhal reinit by sifengtian 20230711 end

int init()
{
    int i;
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int  prop_value = 0, power_on_init = 0;

    if (!nIsReady) {
        ALOGI("perfservice ver:%s", VERSION);

        if(check_vendor_partition()!=0)
            return 0;

        if (trace_init() < 0) {
            ALOGD("trace_init fail");
        } else
            ALOGD("trace_init ok");

        /* check HMP support */
        checkDrvSupport(&gtDrvInfo);
        //if(gtDrvInfo.sysfs == 0 && gtDrvInfo.dvfs == 0) // /sys/devices/system/cpu/possible is not existed
        //    return 0;
        if (CpuFreqReady == 1 && getCputopoInfo(gtDrvInfo.hmp, &nCpuNum, &nClusterNum, &ptClusterTbl, &ptCoreTbl) != 0)
            return 0;

        /* temp for D3 */
        if (nClusterNum == 1) gtDrvInfo.hmp = 0;

        initCPUFreqInfo();

        for (i = 0; i < STATUS_API_COUNT; i ++)
            powerhalAPI_status[i] = 1;

        ALOGI("[init] HintRscList_init");
        HintRscList_init();
        // allocate memory for ptScnList
        if ((ptScnList = (tScnNode*)malloc(sizeof(tScnNode) * (SCN_APP_RUN_BASE))) == NULL) {
            free(ptClusterTbl);
            ALOGE("Can't allocate memory");
            return 0;
        }
        memset(ptScnList, 0, sizeof(tScnNode)*(SCN_APP_RUN_BASE));

        /*if(gtDrvInfo.turbo && (0 == stat(CUS_CONFIG_TABLE_T, &stat_buf)))
            loadConTable(CUS_CONFIG_TABLE_T);
        else*/
        loadConTable(CUS_CONFIG_TABLE);
        update_default_core_min();

        /* Is it the first init during power on */
        property_get(POWER_PROP_INIT, prop_content, "0"); // init before ?
        prop_value = strtol(prop_content, NULL, 10);           // prop_value:1 means powerhal init before
        power_on_init = (prop_value == 1) ? 0 : 1; // power_on_init:1 means init during power on
        if(prop_value == 0) {
            property_set(POWER_PROP_INIT, "1");
        }

        /* resouce config table */
        gRscCtlTblLen = sizeof(RscCfgTbl) / sizeof(*RscCfgTbl);
        ALOGI("[init] loadRscTable:%d", gRscCtlTblLen);
        if ((gRscCtlTbl = (tRscCtlEntry*)malloc(sizeof(tRscCtlEntry) * gRscCtlTblLen)) == NULL) {
            ALOGE("Can't allocate memory");
            free(ptClusterTbl);
            free(ptScnList);
            return 0;
        }
        ALOGI("[init] loadRscTable:%d memset", gRscCtlTblLen);
        memset(gRscCtlTbl, 0, sizeof(tRscCtlEntry)*gRscCtlTblLen);
        loadRscTable(power_on_init);

        // empty list for user registration
        nUserScnBase = 0;
        ALOGI("[init] nUserScnBase:%d", nUserScnBase);
        for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
            resetScenario(i, 1);
        }

        ALOGI("[init] updateCusScnTable:%s", CUS_SCN_TABLE);

        updateCusScnTable(CUS_SCN_TABLE);
        loadPowerDlTable(POWER_DL_XML_DATA);

        /* Message Handler */
        createPowerMsgHandler();

        /* test mode */
        property_get(POWER_PROP_ENABLE, prop_content, "1");
        nPowerLibEnable = strtol(prop_content, NULL, 10);

        if(load_mbrain_api() < 0)
            isMbrainSupport = 0;
        else
            isMbrainSupport = 1;
        LOG_I("isMbrainSupport %d", isMbrainSupport);

        LOG_I("Disable PowerHAL API during boot time.");
        setAPIstatus(STATUS_PERFLOCKACQ_ENABLED, 0);
        setAPIstatus(STATUS_CUSLOCKHINT_ENABLED, 0);
        setAPIstatus(STATUS_APPLIST_ENABLED, 0);

        /* Boot handler */
        createBootHandler();

        nIsReady = 1;
    }
    return 1;
}

static int check_vendor_partition()
{
    char value[PROPERTY_VALUE_MAX] = "\0";
    const char *sname = "/data/vendor/powerhal";

    /* wait for encrypted done */
    property_get("ro.crypto.state", value, "");
    if(!strcmp(value, "")) {
        ALOGI("not ready (crypro)");
        return -1;
    }

    if(strcmp(value, "encrypted") != 0) {
        ALOGI("Device is unencrypted");
        return 0;
    }

    /* ro.crypto.state == "encrypted" */
    property_get("ro.crypto.type", value, "");
    if (!strcmp(value, "file")) {
        ALOGI("FBE feature is open, waiting for decrypt done");
        if (access(sname, F_OK|R_OK|W_OK) != 0) {
            ALOGI("access fail path:%s", sname);
            return -1;
        }
    }
    else {
        property_get("vold.decrypt", value, "");
        if (strcmp(value, "trigger_restart_framework") != 0) {
            ALOGI("not ready (vold)");
            return -1;
        }
    }
    ALOGI("check_vendor_partition done");
    return 0;
}

void setClusterCores(int clusterNum, int totalCore, int *pCoreTbl, int *pMaxCoreTbl)
{
    int coreToSet = 0, maxToSet = 0;
    int i;
    char str[128], buf[32];

    //if(gtDrvInfo.acao)
    //    return;

    for (i=0; i<clusterNum; i++)
        ALOGD("total:%d, cores[%d]:%d, %d", totalCore, i, pCoreTbl[i], pMaxCoreTbl[i]);

    if (gtDrvInfo.perfmgrCpu) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(snprintf(buf, sizeof(buf), "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_BOOST_CORE_CTRL, str);
        ALOGI("cpu_ctrl set cpu core: %s", str);
    }
    else if (gtDrvInfo.perfmgrLegacy) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(snprintf(buf, sizeof(buf), "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_PERFMGR_CORE_CTRL, str);
        ALOGI("legacy set cpu core: %s", str);
    }
    else if (gtDrvInfo.ppmAll) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(snprintf(buf, sizeof(buf), "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_PPM_CORE_CTRL, str);
        ALOGI("ppmall set cpu core: %s", str);
    }
    else if (gtDrvInfo.ppmSupport) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(snprintf(buf, sizeof(buf), "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
            set_value(PATH_PPM_CORE_BASE, i, coreToSet);
            set_value(PATH_PPM_CORE_LIMIT, i, maxToSet);
        }
        str[strlen(str)-1] = '\0'; // remove last space
        ALOGI("ppmsupport set cpu core: %s", str);
    }
    else if (check_core_ctl_ioctl() == 0) {
        str[0] = '\0';
        for (i = 0; i < clusterNum; i ++) {
            coreToSet = (pCoreTbl[i] < 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];

            if(snprintf(buf, sizeof(buf), "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));

            if (coreToSet == PPM_IGNORE) {
                if (maxToSet == PPM_IGNORE)
                    coreToSet = default_core_min[i]; // no one sets core_limit, set core_min to default value
                else
                    coreToSet = 0;
            }
            if (maxToSet == PPM_IGNORE)
                maxToSet = ptClusterTbl[i].cpuNum;

            _CORE_CTL_PACKAGE msg;
            msg.cid = i;
            msg.min = coreToSet;
            msg.max = maxToSet;
            LOG_I("[CORE_CTL_SET_LIMIT_CPUS] cid:%d min:%d max:%d", msg.cid, msg.min, msg.max);
            ioctl(core_ctl_dev_fd, CORE_CTL_SET_LIMIT_CPUS, &msg);
        }
        LOG_I("set core_ctrl: %s", str);
    }

}

void recordCPUFreqInfo(int scenario) {
    char cpufreqSet_str[512], cpufreqSet_buf[512];
    if(cpufreqNeedUpdate || cpufreqHardNeedUpdate) {
        for(int i=0; i >= 0 && i < nCpuNum && i < CPU_CORE_MAX; i++) {
            if(ptCoreTbl[i].byCore == 1) {
                if (snprintf(cpufreqSet_buf, sizeof(cpufreqSet_buf), "cpu[%d] min(%s): %d max(%s): %d ",
                i, freq_to_set[i].curMinFreqSetterName.c_str(), freq_to_set[i].min,
                freq_to_set[i].curMaxFreqSetterName.c_str(), freq_to_set[i].max) < 0) {
                    ALOGE("sprintf error");
                    return;
                }
                LOG_D("cpu[%d] min(%d, %d): %d max(%d, %d): %d", i,
                freq_to_set[i].curMinFreqSetter, freq_to_set[i].curMinFreqSetterHint, freq_to_set[i].min,
                freq_to_set[i].curMaxFreqSetter, freq_to_set[i].curMaxFreqSetterHint, freq_to_set[i].max);
            }else{
                if (snprintf(cpufreqSet_buf, sizeof(cpufreqSet_buf), "cluster[%d] min(%s): %d max(%s): %d ",
                ptCoreTbl[i].cluster, freq_to_set[i].curMinFreqSetterName.c_str(), freq_to_set[i].min,
                freq_to_set[i].curMaxFreqSetterName.c_str(), freq_to_set[i].max) < 0) {
                    ALOGE("sprintf error");
                    return;
                }
                LOG_D("cluster[%d] min(%d, %d): %d max(%d, %d): %d", ptCoreTbl[i].cluster,
                freq_to_set[i].curMinFreqSetter, freq_to_set[i].curMinFreqSetterHint, freq_to_set[i].min,
                freq_to_set[i].curMaxFreqSetter, freq_to_set[i].curMaxFreqSetterHint, freq_to_set[i].max);
                i = i + ptClusterTbl[ptCoreTbl[i].cluster].cpuNum - 1;
            }

            strncat(cpufreqSet_str, cpufreqSet_buf, strlen(cpufreqSet_buf));
        }
        cpufreqSet_str[strlen(cpufreqSet_str)-1] = '\0'; // remove last space
        LOG_I("set %s", cpufreqSet_str);
        notifyCap(scenario, cpufreqNeedUpdate ? "soft" : "hard");
        cpufreqNeedUpdate = 0;
        cpufreqHardNeedUpdate = 0;
    }
}

void initCPUFreqInfo() {
    for(int i=0; i<nCpuNum; i++) {
        soft_core_freq[i].min = 0;
        soft_core_freq[i].max = ptClusterTbl[ptCoreTbl[i].cluster].freqMax;
        soft_core_freq[i].curMinFreqSetter = -1;
        soft_core_freq[i].curMaxFreqSetter = -1;
        soft_core_freq[i].curMinFreqSetterName = "-1";
        soft_core_freq[i].curMaxFreqSetterName = "-1";
        soft_core_freq[i].curMinFreqSetterHint = -1;
        soft_core_freq[i].curMaxFreqSetterHint = -1;
        freq_to_set[i].curMinFreqSetter = -1;
        freq_to_set[i].curMaxFreqSetter = -1;
        freq_to_set[i].curMinFreqSetterName = "-1";
        freq_to_set[i].curMaxFreqSetterName = "-1";
        freq_to_set[i].curMinFreqSetterHint = -1;
        freq_to_set[i].curMaxFreqSetterHint = -1;
    }

    for (int i = 0; i < nClusterNum; i++) {
        soft_freq[i].min = 0;
        soft_freq[i].max = ptClusterTbl[i].freqMax;
        soft_freq[i].curMinFreqSetter = -1;
        soft_freq[i].curMaxFreqSetter = -1;
        soft_freq[i].curMinFreqSetterName = "-1";
        soft_freq[i].curMaxFreqSetterName = "-1";
        soft_freq[i].curMinFreqSetterHint = -1;
        soft_freq[i].curMaxFreqSetterHint = -1;
        hard_freq[i].min = 0;
        hard_freq[i].max = ptClusterTbl[i].freqMax;
        hard_freq[i].curMinFreqSetter = -1;
        hard_freq[i].curMaxFreqSetter = -1;
        hard_freq[i].curMinFreqSetterName = "-1";
        hard_freq[i].curMaxFreqSetterName = "-1";
        hard_freq[i].curMinFreqSetterHint = -1;
        hard_freq[i].curMaxFreqSetterHint = -1;
    }
}

void updateCPUSettingInfo(int cluster, int cpu, int freqToSet, int curMinFreqSetter, string curMinFreqSetterName,
    int curMinFreqSetterHint, int maxToSet, int curMaxFreqSetter, string curMaxFreqSetterName, int curMaxFreqSetterHint) {
    if(cluster < 0 || cpu < 0 || cluster >= CLUSTER_MAX || cpu >= CPU_CORE_MAX) {
        LOG_E("Idx error! cluster: %d, cpu: %d", cluster, cpu);
        return;
    }

    int final_min_freq = (freqToSet <= ptClusterTbl[cluster].freqMin) ? PPM_IGNORE : freqToSet;
    int final_max_freq = (maxToSet <= 0 || ptClusterTbl[cluster].freqMax <= maxToSet) ? PPM_IGNORE : maxToSet;

    freq_to_set[cpu].min = final_min_freq;
    freq_to_set[cpu].curMinFreqSetter = curMinFreqSetter;
    freq_to_set[cpu].curMinFreqSetterName = curMinFreqSetterName;
    freq_to_set[cpu].curMinFreqSetterHint = curMinFreqSetterHint;
    freq_to_set[cpu].max = final_max_freq;
    freq_to_set[cpu].curMaxFreqSetter = curMaxFreqSetter;
    freq_to_set[cpu].curMaxFreqSetterName = curMaxFreqSetterName;
    freq_to_set[cpu].curMaxFreqSetterHint = curMaxFreqSetterHint;

    trace_cpu_freq(cpu, final_min_freq, final_max_freq, curMinFreqSetter, curMaxFreqSetter, curMinFreqSetterHint, curMaxFreqSetterHint);
}

void updateCPUFreqInfo(int cluster, int cpu) {
    if(cluster < 0 || cpu < 0 || cluster >= CLUSTER_MAX || cpu >= CPU_CORE_MAX) {
        LOG_E("Idx error! cluster: %d, cpu: %d", cluster, cpu);
        return;
    }
    soft_core_freq[cpu].curMinFreqSetter = gRscCtlTbl[soft_core_freq[cpu].rscTblMinIdx].curSetter;
    soft_core_freq[cpu].curMaxFreqSetter = gRscCtlTbl[soft_core_freq[cpu].rscTblMaxIdx].curSetter;
    soft_core_freq[cpu].curMinFreqSetterName = gRscCtlTbl[soft_core_freq[cpu].rscTblMinIdx].curSetterName;
    soft_core_freq[cpu].curMaxFreqSetterName = gRscCtlTbl[soft_core_freq[cpu].rscTblMaxIdx].curSetterName;
    soft_core_freq[cpu].curMinFreqSetterHint = gRscCtlTbl[soft_core_freq[cpu].rscTblMinIdx].curSetterHint;
    soft_core_freq[cpu].curMaxFreqSetterHint = gRscCtlTbl[soft_core_freq[cpu].rscTblMaxIdx].curSetterHint;
    soft_core_freq[cpu].min = ptCoreTbl[cpu].freqMinNow;
    soft_core_freq[cpu].max = (ptCoreTbl[cpu].freqMaxNow >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : ptCoreTbl[cpu].freqMaxNow;
    // if max < min => align ceiling with floor
    if(soft_core_freq[cpu].max < soft_core_freq[cpu].min) {
        soft_core_freq[cpu].curMaxFreqSetter = soft_core_freq[cpu].curMinFreqSetter;
        soft_core_freq[cpu].curMaxFreqSetterName = soft_core_freq[cpu].curMinFreqSetterName;
        soft_core_freq[cpu].curMaxFreqSetterHint =  soft_core_freq[cpu].curMinFreqSetterHint;
        soft_core_freq[cpu].max = soft_core_freq[cpu].min;
    }
}

void updateClusterFreqInfo(int cluster) {
    if(cluster < 0 || cluster >= CLUSTER_MAX)
        return;

    soft_freq[cluster].curMinFreqSetter = gRscCtlTbl[soft_freq[cluster].rscTblMinIdx].curSetter;
    soft_freq[cluster].curMaxFreqSetter = gRscCtlTbl[soft_freq[cluster].rscTblMaxIdx].curSetter;
    soft_freq[cluster].curMinFreqSetterName = gRscCtlTbl[soft_freq[cluster].rscTblMinIdx].curSetterName;
    soft_freq[cluster].curMaxFreqSetterName = gRscCtlTbl[soft_freq[cluster].rscTblMaxIdx].curSetterName;
    soft_freq[cluster].curMinFreqSetterHint = gRscCtlTbl[soft_freq[cluster].rscTblMinIdx].curSetterHint;
    soft_freq[cluster].curMaxFreqSetterHint = gRscCtlTbl[soft_freq[cluster].rscTblMaxIdx].curSetterHint;
    soft_freq[cluster].min = (ptClusterTbl[cluster].freqMinNow >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : ptClusterTbl[cluster].freqMinNow;
    soft_freq[cluster].max = (ptClusterTbl[cluster].freqMaxNow >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : ptClusterTbl[cluster].freqMaxNow;
    // if max < min => align ceiling with floor
    if(soft_freq[cluster].max < soft_freq[cluster].min) {
        soft_freq[cluster].curMaxFreqSetter = soft_freq[cluster].curMinFreqSetter;
        soft_freq[cluster].curMaxFreqSetterName = soft_freq[cluster].curMinFreqSetterName;
        soft_freq[cluster].curMaxFreqSetterHint =  soft_freq[cluster].curMinFreqSetterHint;
        soft_freq[cluster].max = soft_freq[cluster].min;
    }

    hard_freq[cluster].curMinFreqSetter = gRscCtlTbl[hard_freq[cluster].rscTblMinIdx].curSetter;
    hard_freq[cluster].curMaxFreqSetter = gRscCtlTbl[hard_freq[cluster].rscTblMaxIdx].curSetter;
    hard_freq[cluster].curMinFreqSetterName = gRscCtlTbl[hard_freq[cluster].rscTblMinIdx].curSetterName;
    hard_freq[cluster].curMaxFreqSetterName = gRscCtlTbl[hard_freq[cluster].rscTblMaxIdx].curSetterName;
    hard_freq[cluster].curMinFreqSetterHint = gRscCtlTbl[hard_freq[cluster].rscTblMinIdx].curSetterHint;
    hard_freq[cluster].curMaxFreqSetterHint = gRscCtlTbl[hard_freq[cluster].rscTblMaxIdx].curSetterHint;
    hard_freq[cluster].min = (ptClusterTbl[cluster].freqHardMinNow >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : ptClusterTbl[cluster].freqHardMinNow;
    hard_freq[cluster].max = (ptClusterTbl[cluster].freqHardMaxNow >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : ptClusterTbl[cluster].freqHardMaxNow;
    // if max < min => align ceiling with floor
    if(hard_freq[cluster].max < hard_freq[cluster].min) {
        hard_freq[cluster].curMaxFreqSetter = hard_freq[cluster].curMinFreqSetter;
        hard_freq[cluster].curMaxFreqSetterName = hard_freq[cluster].curMinFreqSetterName;
        hard_freq[cluster].curMaxFreqSetterHint = hard_freq[cluster].curMinFreqSetterHint;
        hard_freq[cluster].max = hard_freq[cluster].min;
    }
    LOG_V("cluster_%d soft:%d %d, soft setter: %s %d %s %d, hard:%d %d, hard setter: %s %d %s %d",
        cluster,  soft_freq[cluster].min, soft_freq[cluster].max,
        soft_freq[cluster].curMinFreqSetterName.c_str(), soft_freq[cluster].curMinFreqSetter, soft_freq[cluster].curMaxFreqSetterName.c_str(), soft_freq[cluster].curMaxFreqSetter,
        hard_freq[cluster].min, hard_freq[cluster].max,
        hard_freq[cluster].curMinFreqSetterName.c_str(), hard_freq[cluster].curMinFreqSetter, hard_freq[cluster].curMaxFreqSetterName.c_str(), hard_freq[cluster].curMaxFreqSetter);
}

void setClusterHardFreq(int cluster)
{
    int freqToSet = 0, maxToSet = 0;
    int i;
    char str[128], buf[32];

    if (gtDrvInfo.hard_user_limit) {
        str[0] = '\0';
        for (i=0; i<nClusterNum; i++) {
            freqToSet = (ptClusterTbl[i].freqHardMinNow <= 0) ? PPM_IGNORE : ptClusterTbl[i].freqHardMinNow;
            maxToSet = (ptClusterTbl[i].freqHardMaxNow <= 0 || ptClusterTbl[i].freqHardMaxNow >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : ptClusterTbl[i].freqHardMaxNow;
            if(snprintf(buf, sizeof(buf), "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_HARD_USER_LIMIT, str);
        ALOGI("hard user limit set cpu freq: %s", str);
    }
    else {
#if HARDLIMIT_ENABLED
        str[0] = '\0';
        for (i=0; i<nClusterNum; i++) {
            freqToSet = (ptClusterTbl[i].freqHardMinNow <= 0) ? PPM_IGNORE : ptClusterTbl[i].freqHardMinNow;
            maxToSet = (ptClusterTbl[i].freqHardMaxNow <= 0 || ptClusterTbl[i].freqHardMaxNow >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : ptClusterTbl[i].freqHardMaxNow;
            if(snprintf(buf, sizeof(buf), "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0';
        LOG_I("hard userlimit set cpu freq: %s", str);
        setClusterFreq(cluster);
#else
        LOG_E("Hard userlimit Not Supported.");
#endif
    }
}

void setCPUFreq(int cluster, int cpu)
{
    int cluseterFreq = 0, cpuFreq = 0, maxClusterFreq = 0, maxCPUFreq = 0, freqToSet = 0, maxToSet = 0;
    int curMinFreqSetter = -1, curMaxFreqSetter = -1, curMinFreqSetterHint = -1, curMaxFreqSetterHint = -1;
    string curMinFreqSetterName, curMaxFreqSetterName;
    int i;
    char str[128], buf[32];

    if(cluster < 0 || cpu < 0 || cluster >= CLUSTER_MAX || cpu >= CPU_CORE_MAX) {
        LOG_E("Idx error! cluster: %d, cpu: %d", cluster, cpu);
        return;
    }

    if (gtDrvInfo.proc_powerhal_cpu_ctrl) {
        if(ptCoreTbl[cpu].byCore == 1)
            updateCPUFreqInfo(cluster, cpu);

        // cpu freq floor setting
        cpuFreq = (soft_core_freq[cpu].min <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_core_freq[cpu].min >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_core_freq[cpu].min);
        cluseterFreq = (soft_freq[cluster].min <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_freq[cluster].min >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_freq[cluster].min);
        curMinFreqSetter = soft_core_freq[cpu].curMinFreqSetter;
        curMinFreqSetterName = soft_core_freq[cpu].curMinFreqSetterName;
        curMinFreqSetterHint = soft_core_freq[cpu].curMinFreqSetterHint;

        // if cpu freq belongs to the cluster, find the max cpu freq in the cluster for the floor
        if(ptCoreTbl[cpu].byCore == 0) {
            for(i=0; i<ptClusterTbl[cluster].cpuNum; i++) {
                int clusterCPU = ptClusterTbl[cluster].cpuFirstIndex + i;
                if(clusterCPU < 0 || clusterCPU >= CPU_CORE_MAX) {
                    LOG_E("idx error! clusterCPU: %d", clusterCPU);
                    break;
                }
                int clusterCPUFreq = (soft_core_freq[clusterCPU].min <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_core_freq[clusterCPU].min >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_core_freq[clusterCPU].min);
                if(clusterCPUFreq < 0) {
                    LOG_E("idx error! clusterCPUFreq: %d", clusterCPUFreq);
                    break;
                }

                LOG_V("cluster: %d, cpuFreq: %d, clusterCPUFreq: %d", clusterCPU, cpuFreq, clusterCPUFreq);
                if(clusterCPUFreq > cpuFreq) {
                    cpuFreq = clusterCPUFreq;
                    curMinFreqSetter = soft_core_freq[clusterCPU].curMinFreqSetter;
                    curMinFreqSetterName = soft_core_freq[clusterCPU].curMinFreqSetterName;
                    curMinFreqSetterHint = soft_core_freq[clusterCPU].curMinFreqSetterHint;
                }
            }
        }

        freqToSet = (cpuFreq > cluseterFreq) ? cpuFreq : cluseterFreq;
        curMinFreqSetter = (cpuFreq > cluseterFreq) ? curMinFreqSetter : soft_freq[cluster].curMinFreqSetter;
        curMinFreqSetterName = (cpuFreq > cluseterFreq) ? curMinFreqSetterName : soft_freq[cluster].curMinFreqSetterName;
        curMinFreqSetterHint = (cpuFreq > cluseterFreq) ? curMinFreqSetterHint : soft_freq[cluster].curMinFreqSetterHint;

        // cpu freq ceiling setting
        if (soft_core_freq[cpu].max <= 0)
            maxCPUFreq = ptClusterTbl[cluster].freqMax;
        else
            maxCPUFreq = (soft_core_freq[cpu].max <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_core_freq[cpu].max >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_core_freq[cpu].max);

        if (soft_freq[cluster].max <= 0)
            maxClusterFreq = ptClusterTbl[cluster].freqMax;
        else
            maxClusterFreq = (soft_freq[cluster].max <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_freq[cluster].max >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_freq[cluster].max);

        curMaxFreqSetter = soft_core_freq[cpu].curMaxFreqSetter;
        curMaxFreqSetterName = soft_core_freq[cpu].curMaxFreqSetterName;
        curMaxFreqSetterHint = soft_core_freq[cpu].curMaxFreqSetterHint;

        // if cpu freq belongs to the cluster, find the max cpu freq in the cluster for the ceiling
        if(ptCoreTbl[cpu].byCore == 0) {
            for(i=0; i<ptClusterTbl[cluster].cpuNum; i++) {
                int maxClusterCPU = ptClusterTbl[cluster].cpuFirstIndex + i;
                if(maxClusterCPU < 0 || maxClusterCPU >= CPU_CORE_MAX) {
                    LOG_E("idx error! maxClusterCPU: %d", maxClusterCPU);
                    break;
                }

                int maxClusterCPUFreq = (soft_core_freq[maxClusterCPU].max <= ptClusterTbl[cluster].freqMin) ? ptClusterTbl[cluster].freqMin : ((soft_core_freq[maxClusterCPU].max >= ptClusterTbl[cluster].freqMax) ? ptClusterTbl[cluster].freqMax : soft_core_freq[maxClusterCPU].max);
                if(maxClusterCPUFreq < 0) {
                    LOG_E("idx error! maxClusterCPUFreq: %d", maxClusterCPUFreq);
                    break;
                }

                LOG_V("cluster: %d, maxCPUFreq: %d, maxClusterCPUFreq: %d", maxClusterCPU, maxCPUFreq, maxClusterCPUFreq);
                if(maxClusterCPUFreq < maxCPUFreq) {
                    maxCPUFreq = maxClusterCPUFreq;
                    curMaxFreqSetter = soft_core_freq[maxClusterCPU].curMaxFreqSetter;
                    curMaxFreqSetterHint = soft_core_freq[maxClusterCPU].curMaxFreqSetterHint;
                }
            }
        }

        LOG_D("cpuFreq: %d, cluseterFreq: %d, maxCPUFreq: %d, maxClusterFreq: %d", cpuFreq, cluseterFreq, maxCPUFreq, maxClusterFreq);

        // compare with cluster freq and core freq
        maxToSet = (maxCPUFreq < maxClusterFreq) ? maxCPUFreq : maxClusterFreq;
        curMaxFreqSetter = (maxCPUFreq < maxClusterFreq) ? curMaxFreqSetter : soft_freq[cluster].curMaxFreqSetter;
        curMaxFreqSetterName = (maxCPUFreq < maxClusterFreq) ? curMaxFreqSetterName : soft_freq[cluster].curMaxFreqSetterName;
        curMaxFreqSetterHint = (maxCPUFreq < maxClusterFreq) ? curMaxFreqSetterHint : soft_freq[cluster].curMaxFreqSetterHint;

        // if max < min => align ceiling with floor
        if(maxToSet < freqToSet) {
            maxToSet = freqToSet;
            curMaxFreqSetter = curMinFreqSetter;
            curMaxFreqSetterName = curMinFreqSetterName;
            curMaxFreqSetterHint = curMinFreqSetterHint;
        }

#if HARDLIMIT_ENABLED
        // compare with hardlimit
        if (freqToSet < hard_freq[cluster].min) {
            freqToSet = hard_freq[cluster].min;
            curMinFreqSetter = hard_freq[cluster].curMinFreqSetter;
            curMinFreqSetterName = hard_freq[cluster].curMinFreqSetterName;
            curMinFreqSetterHint = hard_freq[cluster].curMinFreqSetterHint;
        }
        if (freqToSet > hard_freq[cluster].max) {
            freqToSet = hard_freq[cluster].max;
            curMinFreqSetter = hard_freq[cluster].curMaxFreqSetter;
            curMinFreqSetterName = hard_freq[cluster].curMaxFreqSetterName;
            curMinFreqSetterHint = hard_freq[cluster].curMaxFreqSetterHint;
        }
        if (maxToSet > hard_freq[cluster].max) {
            maxToSet = hard_freq[cluster].max;
            curMaxFreqSetter = hard_freq[cluster].curMaxFreqSetter;
            curMaxFreqSetterName = hard_freq[cluster].curMaxFreqSetterName;
            curMaxFreqSetterHint = hard_freq[cluster].curMaxFreqSetterHint;
        }
        if (maxToSet < hard_freq[cluster].min) {
            maxToSet = hard_freq[cluster].min;
            curMaxFreqSetter = hard_freq[cluster].curMinFreqSetter;
            curMaxFreqSetterName = hard_freq[cluster].curMinFreqSetterName;
            curMaxFreqSetterHint = hard_freq[cluster].curMinFreqSetterHint;
        }
#endif
        if (ptCoreTbl[cpu].byCore == 0)
            cpu = ptClusterTbl[cluster].cpuFirstIndex;

        updateCPUSettingInfo(cluster, cpu, freqToSet, curMinFreqSetter, curMinFreqSetterName, curMinFreqSetterHint,
            maxToSet, curMaxFreqSetter, curMaxFreqSetterName, curMaxFreqSetterHint);

        if(snprintf(buf, sizeof(buf), "%d %d %d ", cpu, freqToSet, maxToSet) < 0) {
            ALOGE("sprintf error");
            return;
        }
        strncat(str, buf, strlen(buf));

        str[strlen(str)-1] = '\0'; // remove last space

        LOG_D("set cpu freq: %s", str);

        if (set_value(PATH_PROC_CPU_CTRL, str) == 0) {
            notifyWriteFail();
        }
    }
}

void setClusterFreq(int cluster)
{
    if(cluster >= nClusterNum) return;

    int i;

    updateClusterFreqInfo(cluster);

    LOG_V("set cluster %d cpu freq", cluster);

    int clusterFirstCPU = ptClusterTbl[cluster].cpuFirstIndex;
    for(i=clusterFirstCPU; i < (clusterFirstCPU + ptClusterTbl[cluster].cpuNum); i++) {
        setCPUFreq(cluster, i);
        // The cluster freq is set, so we don't need to set the freq of other CPUs
        if(ptCoreTbl[i].byCore == 0) break;
    }
}

void checkDrvSupport(tDrvInfo *ptDrvInfo)
{
    struct stat stat_buf;
    int ppmCore;

    ptDrvInfo->perfmgrCpu = (0 == stat(PATH_BOOST_FREQ_CTRL, &stat_buf)) ? 1 : 0;
    ptDrvInfo->perfmgrLegacy = (0 == stat(PATH_PERFMGR_FREQ_CTRL, &stat_buf)) ? 1 : 0;
    ptDrvInfo->ppmSupport = (0 == stat(PATH_PPM_FREQ_LIMIT, &stat_buf)) ? 1 : 0;
    ptDrvInfo->proc_powerhal_cpu_ctrl = (0 == stat(PATH_PROC_CPU_CTRL, &stat_buf)) ? 1 : 0;
    ptDrvInfo->ppmAll = (0 == stat(PATH_PPM_FREQ_CTRL, &stat_buf)) ? 1 : 0;
    if(0 == stat(PATH_PERFMGR_TOPO_CHECK_HMP, &stat_buf))
        ptDrvInfo->hmp = (get_int_value(PATH_PERFMGR_TOPO_CHECK_HMP)==1) ? 1 : 0;
    else
        ptDrvInfo->hmp = (get_int_value(PATH_CPUTOPO_CHECK_HMP)==1) ? 1 : 0;

    ptDrvInfo->sysfs = (0 == stat(PATH_CPU0_CPUFREQ, &stat_buf)) ? 1 : 0;
    ptDrvInfo->dvfs = (0 == stat(PATH_CPUFREQ_ROOT, &stat_buf)) ? 1 : 0;
    CpuFreqReady = (ptDrvInfo->sysfs == 1) ? 1 : 0;

    ptDrvInfo->hard_user_limit = (0 == stat(PATH_HARD_USER_LIMIT, &stat_buf)) ? 1 : 0;
    if(ptDrvInfo->hard_user_limit == 0) {
        ALOGV("checkDrvSupport hard user limit failed: %s\n", strerror(errno));
    }

    ppmCore = (0 == stat(PATH_PPM_CORE_LIMIT, &stat_buf)) ? 1 : 0;
    if(ptDrvInfo->ppmSupport)
        ptDrvInfo->acao = (ppmCore) ? 0 : 1; // PPM not support core => ACAO
    else
        ptDrvInfo->acao = 0; // no PPM => no ACAO

    /* check file node first */
    ptDrvInfo->turbo = (0 == stat(PATH_TURBO_SUPPORT, &stat_buf)) ? 1 : 0;
    if(ptDrvInfo->turbo == 1) {
        ptDrvInfo->turbo = (get_int_value(PATH_TURBO_SUPPORT)==1) ? 1 : 0;
    }

    ALOGI("checkDrvSupport - perfmgr:%d/%d, ppm:%d, ppmAll:%d, acao:%d, hmp:%d, sysfs:%d, dvfs:%d, turbo:%d, proc_powerhal_cpu_ctrl:%d",
        ptDrvInfo->perfmgrCpu, ptDrvInfo->perfmgrLegacy, ptDrvInfo->ppmSupport, ptDrvInfo->ppmAll,ptDrvInfo->acao, ptDrvInfo->hmp,
        ptDrvInfo->sysfs, ptDrvInfo->dvfs, ptDrvInfo->turbo, ptDrvInfo->proc_powerhal_cpu_ctrl);
}

template <typename T>
size_t get_valid_allocation_size(int* pNum) {
    size_t maxAllowedSize = SIZE_MAX / sizeof(T);

    if(pNum == NULL || *pNum <= 0 || *pNum > maxAllowedSize) {
        ALOGE("Allocated size too big !!!");
        return -1;
    } else {
        return (size_t)(*pNum) * sizeof(T);
    }
}

int getCputopoInfo(int isHmpSupport, int *pnCpuNum, int *pnClusterNum, tClusterInfo **pptClusterTbl, tCoreInfo **pptCoreTbl)
{
    int i, j, k;
    int cpu_num[CLUSTER_MAX], cpu_index[CLUSTER_MAX];
    int cpu_mapping_policy[CPU_CORE_MAX], cluster_policy_num[CLUSTER_MAX];
    int cputopoClusterNum = 1;
    struct stat stat_buf;
    size_t allocationSize = 0;

    for (i=0; i<CLUSTER_MAX; i++) {
        cpu_num[i] = cpu_index[i] = 0;
        cluster_policy_num[i] = 0;
    }

    for (i=0; i<CPU_CORE_MAX; i++)
        cpu_mapping_policy[i] = 0;

    *pnCpuNum = get_cpu_num();

    if(0 == stat(PATH_PERFMGR_TOPO_NR_CLUSTER, &stat_buf))
        cputopoClusterNum = get_int_value(PATH_PERFMGR_TOPO_NR_CLUSTER);
    else if(0 == stat(PATH_CPUTOPO_NR_CLUSTER, &stat_buf))
        cputopoClusterNum = get_int_value(PATH_CPUTOPO_NR_CLUSTER);
    else
        getCputopoFromSysfs(*pnCpuNum, &cputopoClusterNum, NULL, NULL, cpu_mapping_policy);

    //*pnClusterNum = (cputopoClusterNum > 0 && isHmpSupport == 0) ? 1 : cputopoClusterNum; // temp solution for D3
    LOG_V("isHmpSupport:%d", isHmpSupport);

    *pnClusterNum = cputopoClusterNum;
    ALOGI("getCputopoInfo - cpuNum:%d, cluster:%d, cputopoCluster:%d", *pnCpuNum, *pnClusterNum, cputopoClusterNum);

    if((*pnClusterNum) < 0 || (*pnClusterNum) > CLUSTER_MAX) {
        ALOGE("wrong cluster number:%d", *pnClusterNum);
        return -1;
    }

    *pptClusterTbl = (tClusterInfo*)malloc(sizeof(tClusterInfo) * (*pnClusterNum));
    if (*pptClusterTbl  == NULL) {
        ALOGE("Can't allocate memory for pptClusterTbl");
        return -1;
    }

    if(get_cputopo_cpu_info(*pnClusterNum, cpu_num, cpu_index) < 0) {
        getCputopoFromSysfs(*pnCpuNum, &cputopoClusterNum, cpu_num, cpu_index, cpu_mapping_policy);
    }

    allocationSize = get_valid_allocation_size<tCoreInfo>(pnCpuNum);
    if(allocationSize == -1) return -1;

    *pptCoreTbl = (tCoreInfo*)malloc(allocationSize);
    if (*pptCoreTbl  == NULL) {
        ALOGE("Can't allocate memory for pptCoreTbl");
        return -1;
    }

    for (i=0; i<*pnClusterNum; i++) {
        (*pptClusterTbl)[i].cpuNum = cpu_num[i];
        (*pptClusterTbl)[i].cpuFirstIndex = cpu_index[i];
        (*pptClusterTbl)[i].cpuMinNow = -1;
        (*pptClusterTbl)[i].cpuMaxNow = cpu_num[i];

        (*pptClusterTbl)[i].freqMin = 0;
        (*pptClusterTbl)[i].freqMax = 0;
        (*pptClusterTbl)[i].freqCount = 0;
        (*pptClusterTbl)[i].pFreqTbl = NULL;

        if (gtDrvInfo.ppmSupport)
            get_ppm_cpu_freq_info(i, &((*pptClusterTbl)[i].freqMax),&((*pptClusterTbl)[i].freqCount), &((*pptClusterTbl)[i].pFreqTbl));
        else
            get_cpu_freq_info(cpu_index[i], &((*pptClusterTbl)[i].freqMax), &((*pptClusterTbl)[i].freqCount), &((*pptClusterTbl)[i].pFreqTbl));
        (*pptClusterTbl)[i].freqMinNow = (*pptClusterTbl)[i].freqHardMinNow = 0;
        (*pptClusterTbl)[i].freqMaxNow = (*pptClusterTbl)[i].freqHardMaxNow = (*pptClusterTbl)[i].freqMax;
        ALOGI("[cluster %d]: cpu:%d, first:%d, freq count:%d, max_freq:%d", i, (*pptClusterTbl)[i].cpuNum, (*pptClusterTbl)[i].cpuFirstIndex, (*pptClusterTbl)[i].freqCount, (*pptClusterTbl)[i].freqMax);
        for (j=0; j<(*pptClusterTbl)[i].freqCount; j++) {
            if (j == 0) {
                ALOGI("  %d: %d", j, (*pptClusterTbl)[i].pFreqTbl[j]);
                (*pptClusterTbl)[i].freqMin = (*pptClusterTbl)[i].pFreqTbl[j];
            } else if (j == (*pptClusterTbl)[i].freqCount - 1)
                ALOGI("  %d: %d", j, (*pptClusterTbl)[i].pFreqTbl[j]);
            else
                ALOGD("  %d: %d", j, (*pptClusterTbl)[i].pFreqTbl[j]);
        }

        (*pptClusterTbl)[i].sysfsMinPath = (char*)malloc(sizeof(char) * 128);
        if ((*pptClusterTbl)[i].sysfsMinPath != NULL) {
            if(snprintf((*pptClusterTbl)[i].sysfsMinPath, 127, "/sys/devices/system/cpu/cpufreq/policy%d/scaling_min_freq", (*pptClusterTbl)[i].cpuFirstIndex) < 0) {
                ALOGE("snprint error");
                return -1;
            }
            (*pptClusterTbl)[i].sysfsMinPath[127] = '\0';
        }

        (*pptClusterTbl)[i].sysfsMaxPath = (char*)malloc(sizeof(char) * 128);
        if ((*pptClusterTbl)[i].sysfsMaxPath != NULL) {
            if(snprintf((*pptClusterTbl)[i].sysfsMaxPath, 127, "/sys/devices/system/cpu/cpufreq/policy%d/scaling_max_freq", (*pptClusterTbl)[i].cpuFirstIndex) < 0) {
                ALOGE("snprint error");
                return -1;
            }
            (*pptClusterTbl)[i].sysfsMaxPath[127] = '\0';
        }

        int prePolicy = -1;
        cluster_policy_num[i] = 0;
        for(j = (*pptClusterTbl)[i].cpuFirstIndex; j < ((*pptClusterTbl)[i].cpuFirstIndex + (*pptClusterTbl)[i].cpuNum); j++) {
            (*pptCoreTbl)[j].policy = cpu_mapping_policy[j];
            (*pptCoreTbl)[j].cluster = i;
            (*pptCoreTbl)[j].freqMinNow = 0;
            (*pptCoreTbl)[j].freqMaxNow = (*pptClusterTbl)[i].freqMax;

            if(cpu_mapping_policy[j] != prePolicy) cluster_policy_num[i]++;
            prePolicy = cpu_mapping_policy[j];
            ALOGD("core table: %d %d %d %d, prePolicy: %d", j, i, (*pptCoreTbl)[j].freqMaxNow, (*pptClusterTbl)[i].freqMax, prePolicy);
        }

        for(j = (*pptClusterTbl)[i].cpuFirstIndex; j < ((*pptClusterTbl)[i].cpuFirstIndex + (*pptClusterTbl)[i].cpuNum); j++) {
            (*pptCoreTbl)[j].byCore = (cluster_policy_num[i] > 1) ? 1 : 0;
            ALOGD("cluster: %d, byCore: %d, cluster policy num: %d", i, (*pptCoreTbl)[j].byCore, cluster_policy_num[i]);
        }
    }
    return 0;
}

inline int checkSuccess(int scenario)
{
    return (nPowerLibEnable && scenario >= 0 && scenario < SCN_APP_RUN_BASE + nPackNum);
}

void GetScnName(int scenario, const char *func_name, char *scn_name)
{
    //LOG_I("[%s] scn:%d scn_type:%d", func_name, scenario, ptScnList[scenario].scn_type);
    int hint;

    if (ptScnList[scenario].scn_type == SCN_POWER_HINT) {
        set_str_cpy(scn_name, ptScnList[scenario].comm, PACK_NAME_MAX);
        ALOGI("[%s] scn:%d hint:%s", func_name, scenario, ptScnList[scenario].comm);
    } else if (ptScnList[scenario].scn_type == SCN_PERF_LOCK_HINT) {
        set_str_cpy(scn_name, ptScnList[scenario].comm, PACK_NAME_MAX);
        ALOGD("[%s] scn:%d hdl:%d lock_user:%s pid:%d tid:%d dur:%d",
            func_name, scenario, ptScnList[scenario].handle_idx, ptScnList[scenario].comm, ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].lock_duration);
    } else if (ptScnList[scenario].scn_type == SCN_PACK_HINT) {
        ALOGI("[%s] scn:%d pack:%s", func_name, scenario, ptScnList[scenario].pack_name);
        set_str_cpy(scn_name, ptScnList[scenario].pack_name, PACK_NAME_MAX);
    } else if (ptScnList[scenario].scn_type == SCN_CUS_POWER_HINT) {
        hint = ptScnList[scenario].cus_lock_hint;
        ALOGD("[%s] scn:%d hdl:%d hint:%d comm:%s pid:%d", func_name, scenario, ptScnList[scenario].handle_idx, hint, ptScnList[scenario].comm, ptScnList[scenario].pid);
        strncpy(scn_name, getHintName(hint).c_str(), PACK_NAME_MAX);
    } else {
        ALOGI("[%s] scn:%d user_type:%d", func_name, scenario, ptScnList[scenario].scn_type);
        set_str_cpy(scn_name, ptScnList[scenario].comm, PACK_NAME_MAX);
    }
}

static void notifyMbrainCap(int pid, int tid, int hdl, int duration, int hintId, int clusterId, int qosType, int freq)
{
    if( (0 != isMbrainSupport) && (nullptr != notifyMbrainCpuFreqCap) ){
        notifyMbrainCpuFreqCap(pid, tid, hdl, duration, hintId, clusterId, qosType, freq);
    }
}

static void notifyMBrainGameMode(bool enabled)
{
    if( (0 != isMbrainSupport) && (nullptr != notifyMBrainGameModeEnabled) ) {
        notifyMBrainGameModeEnabled(enabled);
    }
}

static void notifyCap(int scenario, string type)
{
    for (int i=0; i<nClusterNum; i++) {
        if(0 != isMbrainSupport) {
            notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx,
                ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 1, (type == "hard" ) ? ptClusterTbl[i].freqHardMinNow : ptClusterTbl[i].freqMinNow);
            notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx,
                ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 2, (type == "hard" ) ? ptClusterTbl[i].freqHardMaxNow : ptClusterTbl[i].freqMaxNow);
        }
    }
}

int perfScnEnable(int scenario)
{
    int result, i = 0;
    char scn_name[PACK_NAME_MAX];
    int log_hasI = 1;
    char str[128], buf[128];

    if (checkSuccess(scenario)) {
        //if (STATE_ON == ptScnList[scenario].scn_state)
        //    return 0;

        GetScnName(scenario, "PE", scn_name);
        ptScnList[scenario].scn_state = STATE_ON;

        /*
            scan control table(perfcontable.txt) and judge which setting of scene is beeter
            and then replace it.
            less is meaning system setting less than current scene value is better
            more is meaning system setting more than current scene value is better
        */
        for(int idx = 0; idx < FIELD_SIZE; idx++) {
            //if(tConTable[idx].entry.length() == 0)
            //    break;

            if (tConTable[idx].resetVal !=  ptScnList[scenario].scn_param[idx])
                ALOGD("[perfScnEnable] scn:%s, scn_pr:%d user set cmdName:%s value:%d cur:%d isValid:%d pr:%d",
                    scn_name, ptScnList[scenario].priority, tConTable[idx].cmdName.c_str(), ptScnList[scenario].scn_param[idx],
                    tConTable[idx].curVal, tConTable[idx].isValid, tConTable[idx].priority);

            if(tConTable[idx].isValid == -1)
                continue;

            if(ptScnList[scenario].priority > tConTable[idx].priority)
                continue;

            if(tConTable[idx].comp.compare(LESS) == 0) {
                if(((ptScnList[scenario].scn_param[idx] < tConTable[idx].curVal) && (ptScnList[scenario].priority == tConTable[idx].priority)) ||
                  ((ptScnList[scenario].priority < tConTable[idx].priority) && (ptScnList[scenario].scn_param[idx] != tConTable[idx].resetVal)))
                {
                    tConTable[idx].curVal = ptScnList[scenario].scn_param[idx];
                    tConTable[idx].priority = ptScnList[scenario].priority;
                    if(tConTable[idx].prefix.length() == 0) {
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), ptScnList[scenario].scn_param[idx])) {
                                if(log_hasI)
                                    ALOGI("[PE] %s update cmd:%x, param:%d, pr:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx], tConTable[idx].priority);
                                else
                                    ALOGD("[PE] %s update cmd:%x, param:%d, pr:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx], tConTable[idx].priority);
                            }
                        }
                    } else {
                        char inBuf[64];
                        if(snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), ptScnList[scenario].scn_param[idx]) < 0) {
                            ALOGE("snprint error");
                        }
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), inBuf)) {
                                if(log_hasI)
                                    ALOGI("[PE] (less) %s set +prefix:%s;", tConTable[idx].entry.c_str(), inBuf);
                                else
                                    ALOGD("[PE] (less) %s set +prefix:%s;", tConTable[idx].entry.c_str(), inBuf);
                            }
                        }
                    }
                }
            }
            else {
                if(((ptScnList[scenario].scn_param[idx] > tConTable[idx].curVal) && (ptScnList[scenario].priority == tConTable[idx].priority)) ||
                  ((ptScnList[scenario].priority < tConTable[idx].priority) && (ptScnList[scenario].scn_param[idx] != tConTable[idx].resetVal)))
                {
                    tConTable[idx].curVal = ptScnList[scenario].scn_param[idx];
                    tConTable[idx].priority = ptScnList[scenario].priority;
                    if(tConTable[idx].prefix.length() == 0) {
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), ptScnList[scenario].scn_param[idx])) {
                                if(log_hasI)
                                    ALOGI("[PE] %s update cmd:%x, param:%d, pr:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx], tConTable[idx].priority);
                                else
                                    ALOGD("[PE] %s update cmd:%x, param:%d, pr:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx], tConTable[idx].priority);
                            }
                        }
                    } else {
                        char inBuf[64];
                        if(snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), ptScnList[scenario].scn_param[idx]) < 0) {
                            ALOGE("snprint error");
                        }
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), inBuf)) {
                                if(log_hasI)
                                    ALOGI("[PE] (more) %s set +prefix:%s;", tConTable[idx].entry.c_str(), inBuf);
                                else
                                    ALOGD("[PE] (more) %s set +prefix:%s;", tConTable[idx].entry.c_str(), inBuf);
                            }
                        }
                    }
                }
            }
        }

        set_fpsgo_render_pid(ptScnList[scenario].render_pid, (void*)&ptScnList[scenario]);

        /*Rescontable*/
        for(int idx = 0; idx < gRscCtlTblLen; idx++) {
            int unsetCheck = 0;
            if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                break;

            if (gRscCtlTbl[idx].resetVal != ptScnList[scenario].scn_rsc[idx])
                ALOGD("[perfScnEnable] scn:%s scn_pr:%d user set: cmdName:%s param:%d cur:%d isValid:%d pr:%d",
                    scn_name, ptScnList[scenario].priority, RscCfgTbl[idx].cmdName.c_str(), ptScnList[scenario].scn_rsc[idx],
                    gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].isValid, gRscCtlTbl[idx].priority);

            if(gRscCtlTbl[idx].isValid != 1)
                continue;

            if(ptScnList[scenario].priority > gRscCtlTbl[idx].priority)
                continue;

            if(RscCfgTbl[idx].comp == SMALLEST) {
                if(((ptScnList[scenario].scn_rsc[idx] < gRscCtlTbl[idx].curVal) && (ptScnList[scenario].priority == gRscCtlTbl[idx].priority)) ||
                  ((ptScnList[scenario].priority < gRscCtlTbl[idx].priority) && (ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)))
                {
                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    gRscCtlTbl[idx].curSetter = ptScnList[scenario].pid;
                    gRscCtlTbl[idx].curSetterName = scn_name;
                    gRscCtlTbl[idx].curSetterHint = ptScnList[scenario].cus_lock_hint;
                    gRscCtlTbl[idx].priority = ptScnList[scenario].priority;
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    } else {
                        ALOGD("[PE] %s update cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    }
                }
            } else if(RscCfgTbl[idx].comp == BIGGEST) {
                if(((ptScnList[scenario].scn_rsc[idx] > gRscCtlTbl[idx].curVal) && (ptScnList[scenario].priority == gRscCtlTbl[idx].priority)) ||
                  ((ptScnList[scenario].priority < gRscCtlTbl[idx].priority) && (ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)))
                {
                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    gRscCtlTbl[idx].curSetter = ptScnList[scenario].pid;
                    gRscCtlTbl[idx].curSetterName = scn_name;
                    gRscCtlTbl[idx].curSetterHint = ptScnList[scenario].cus_lock_hint;
                    gRscCtlTbl[idx].priority = ptScnList[scenario].priority;
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    } else {
                        ALOGD("[PE] %s update cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    }
                }
            } else {
                if((ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal) && (ptScnList[scenario].priority <= gRscCtlTbl[idx].priority))
                {
                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if ((scenario != i) && (ptScnList[i].scn_state == STATE_ON) && (ptScnList[scenario].priority < gRscCtlTbl[idx].priority)
                            && (ptScnList[i].oneshotReset == 1) && (ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)) {
                            set_fpsgo_render_pid(ptScnList[i].render_pid, (void*)&ptScnList[i]);
                            result = RscCfgTbl[idx].unset_func(ptScnList[i].scn_rsc[idx], (void*)&ptScnList[i]);
                            ptScnList[i].hasReset = 0;
                            unsetCheck = 1;
                            ALOGI("[PD] %s update ONESHOT(unset) cmd:%x param:%d pr:%d", scn_name, RscCfgTbl[idx].cmdID, ptScnList[i].scn_rsc[idx], ptScnList[i].priority);
                        }
                    }

                    //reset scenario render pid
                    if(unsetCheck)
                        set_fpsgo_render_pid(ptScnList[scenario].render_pid, (void*)&ptScnList[scenario]);

                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    gRscCtlTbl[idx].priority = ptScnList[scenario].priority;
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update ONESHOT(set) cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    } else {
                        ALOGD("[PE] %s update ONESHOT(set) cmd:%x param:%d pr:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].priority);
                    }
                }
            }
        }
    }

    recordCPUFreqInfo(scenario);

    return 0;
}

int perfScnDisable(int scenario)
{
    return perfScnUpdate(scenario, 0);
}

/*
    force_update == 0 => disable scenario
    force_update == 1 => update scenario
 */
int perfScnUpdate(int scenario, int force_update)
{
    int i, j, priority, numToSet, numofCurr, result = 0;
    char scn_name[PACK_NAME_MAX], temp_scn_name[PACK_NAME_MAX];
    char func_name[PACK_NAME_MAX];

    if (checkSuccess(scenario)) {
        if (STATE_OFF == ptScnList[scenario].scn_state)
            return 0;

        set_str_cpy(scn_name, ptScnList[scenario].comm, PACK_NAME_MAX);

        if (force_update) {
            set_str_cpy(func_name, "PU", PACK_NAME_MAX);
            ALOGD("[%s] scn:%d, update", func_name, scenario);
        }
        else {
            set_str_cpy(func_name, "PD", PACK_NAME_MAX);
            GetScnName(scenario, func_name, scn_name);
            ptScnList[scenario].scn_state = STATE_OFF;
        }

        for(int idx = 0; idx < FIELD_SIZE; idx++) {
            //if( tConTable[idx].entry.length() == 0 )
            //    break;

            if(tConTable[idx].isValid == -1)
                continue;

            if(ptScnList[scenario].priority > tConTable[idx].priority)
                continue;

            ALOGV("[%s] scn:%s, cmd:%s, reset:%d, default:%d, cur:%d, param:%d, pr:%d", func_name, scn_name, tConTable[idx].cmdName.c_str(),
                tConTable[idx].resetVal, tConTable[idx].defaultVal, tConTable[idx].curVal, ptScnList[scenario].scn_param[idx], tConTable[idx].priority);
            if(tConTable[idx].comp.compare(LESS) == 0) {
                if(force_update || (ptScnList[scenario].scn_param[idx] < tConTable[idx].resetVal
                        && ptScnList[scenario].scn_param[idx] <= tConTable[idx].curVal)) {
                    numToSet = tConTable[idx].resetVal;
                    priority = PRIORITY_LOWEST;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].scn_param[idx] != tConTable[idx].resetVal) {
                            ALOGD("idx:%d, scn_pr:%d, priority:%d, param: %d",
                                i, ptScnList[i].priority, priority, ptScnList[i].scn_param[idx]);
                            if(ptScnList[i].priority == priority) {
                                numToSet = min(numToSet, ptScnList[i].scn_param[idx]);
                            }else if(ptScnList[i].priority < priority) {
                                numToSet = ptScnList[i].scn_param[idx];
                                priority = ptScnList[i].priority;
                            }
                        }
                    }

                    numofCurr = tConTable[idx].curVal;
                    tConTable[idx].curVal = numToSet;
                    tConTable[idx].priority = priority;

                    if(tConTable[idx].curVal != numofCurr) {
                        if(tConTable[idx].curVal == tConTable[idx].resetVal) {
                            numToSet = tConTable[idx].defaultVal;
                        } else {
                            numToSet = tConTable[idx].curVal;
                        }
                        ALOGI("[%s] %s update cmd:%x, param:%d, pr:%d",
                            func_name, scn_name, tConTable[idx].cmdID, numToSet, tConTable[idx].priority);

                        if(tConTable[idx].entry.length() > 0) {
                            if(tConTable[idx].prefix.length() == 0)
                                set_value(tConTable[idx].entry.c_str(), numToSet);
                            else {
                                char inBuf[64];
                                if(snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), numToSet) < 0) {
                                    ALOGE("snprint error");
                                }
                                if(tConTable[idx].entry.length() > 0)
                                    set_value(tConTable[idx].entry.c_str(), inBuf);
                                ALOGI("[%s] (less) %s set +prefix:%s;", func_name, tConTable[idx].entry.c_str(), inBuf);
                            }
                        }
                    }
                }
            }
            else {
                if(force_update || (ptScnList[scenario].scn_param[idx] > tConTable[idx].resetVal
                        && ptScnList[scenario].scn_param[idx] >= tConTable[idx].curVal)) {
                    numToSet = tConTable[idx].resetVal;
                    priority = PRIORITY_LOWEST;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].scn_param[idx] != tConTable[idx].resetVal) {
                            ALOGD("idx:%d, scn_pr:%d, priority:%d, param: %d",
                                i, ptScnList[i].priority, priority, ptScnList[i].scn_param[idx]);
                            if(ptScnList[i].priority == priority) {
                                numToSet = max(numToSet, ptScnList[i].scn_param[idx]);
                            }else if(ptScnList[i].priority < priority) {
                                numToSet = ptScnList[i].scn_param[idx];
                                priority = ptScnList[i].priority;
                            }
                        }
                    }

                    numofCurr = tConTable[idx].curVal;
                    tConTable[idx].curVal = numToSet;
                    tConTable[idx].priority = priority;

                    if(tConTable[idx].curVal != numofCurr) {
                        if(tConTable[idx].curVal == tConTable[idx].resetVal) {
                            numToSet = tConTable[idx].defaultVal;
                        } else {
                            numToSet = tConTable[idx].curVal;
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d, pr:%d",
                            func_name, scn_name, tConTable[idx].cmdID, numToSet, tConTable[idx].priority);
                        if(tConTable[idx].entry.length() > 0) {
                            if(tConTable[idx].prefix.length() == 0)
                                set_value(tConTable[idx].entry.c_str(), numToSet);
                            else {
                                char inBuf[64];
                                if(snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), numToSet) < 0) {
                                    ALOGE("snprint error");
                                }
                                if(tConTable[idx].entry.length() > 0)
                                    set_value(tConTable[idx].entry.c_str(), inBuf);
                                ALOGI("[%s] (more) %s set +prefix:%s;", func_name, tConTable[idx].entry.c_str(), inBuf);
                            }
                        }
                    }
                }
            }
        }

        set_fpsgo_render_pid(ptScnList[scenario].render_pid, (void*)&ptScnList[scenario]);

        /*Rescontable*/
        for(int idx = 0; idx < gRscCtlTblLen; idx++) {
            ALOGV("[%s] RscCfgTbl[%d] scn:%s, cmd:%s, param:%d",
                func_name, idx, scn_name, RscCfgTbl[idx].cmdName.c_str(), ptScnList[scenario].scn_rsc[idx]);

            if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                break;

            if(gRscCtlTbl[idx].isValid != 1)
                continue;

            if(ptScnList[scenario].priority > gRscCtlTbl[idx].priority)
                continue;

            ALOGV("[%s] RscCtlTbl[%d] scn:%s, cmd:%s, reset:%d, default:%d, cur:%d, param:%d",
                func_name, idx, scn_name, RscCfgTbl[idx].cmdName.c_str(), gRscCtlTbl[idx].resetVal,
                RscCfgTbl[idx].defaultVal, gRscCtlTbl[idx].curVal, ptScnList[scenario].scn_rsc[idx]);

            if (RscCfgTbl[idx].comp == SMALLEST) {
                if(force_update || (ptScnList[scenario].scn_rsc[idx] < gRscCtlTbl[idx].resetVal
                        && ptScnList[scenario].scn_rsc[idx] <= gRscCtlTbl[idx].curVal)) {
                    numToSet = gRscCtlTbl[idx].resetVal;
                    priority = PRIORITY_LOWEST;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].scn_rsc[idx] < gRscCtlTbl[idx].resetVal) {
                            GetScnName(i, func_name, temp_scn_name);
                            ALOGD("idx:%d, cmd:%s, comm:%s, scn_pr:%d, param:%d, pr:%d",
                                i, RscCfgTbl[idx].cmdName.c_str(), temp_scn_name, ptScnList[i].priority, ptScnList[i].scn_rsc[idx], priority);
                            if((ptScnList[i].priority == priority && ptScnList[i].scn_rsc[idx] < numToSet) || (ptScnList[i].priority < priority)) {
                                numToSet = ptScnList[i].scn_rsc[idx];
                                gRscCtlTbl[idx].curSetter = ptScnList[i].pid;
                                gRscCtlTbl[idx].curSetterName = temp_scn_name;
                                gRscCtlTbl[idx].curSetterHint = ptScnList[i].cus_lock_hint;
                                priority = ptScnList[i].priority;
                            }
                        }
                    }

                    if(numToSet == gRscCtlTbl[idx].resetVal) {
                        gRscCtlTbl[idx].curSetter = -1;
                        gRscCtlTbl[idx].curSetterName = "-1";
                        gRscCtlTbl[idx].curSetterHint = -1;
                    }

                    numofCurr = gRscCtlTbl[idx].curVal;
                    gRscCtlTbl[idx].curVal = numToSet;
                    gRscCtlTbl[idx].priority = priority;

                    if(gRscCtlTbl[idx].curVal != numofCurr) {
                        if(gRscCtlTbl[idx].curVal == gRscCtlTbl[idx].resetVal) {
                            numToSet = RscCfgTbl[idx].defaultVal;
                            result = RscCfgTbl[idx].unset_func(numToSet, (void*)&ptScnList[scenario]);
                        } else {
                            numToSet = gRscCtlTbl[idx].curVal;
                            result = RscCfgTbl[idx].set_func(numToSet, (void*)&ptScnList[scenario]);
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d pr:%d",
                            func_name, scn_name, RscCfgTbl[idx].cmdID, numToSet, gRscCtlTbl[idx].priority);
                    }
                    ALOGV("[%s] RscCfgTbl SMALLEST numToSet:%d, ret:%d", func_name, numToSet, result);
                }
            } else if (RscCfgTbl[idx].comp == BIGGEST) {
                if(force_update || (ptScnList[scenario].scn_rsc[idx] > gRscCtlTbl[idx].resetVal
                        && ptScnList[scenario].scn_rsc[idx] >= gRscCtlTbl[idx].curVal)) {
                    numToSet = gRscCtlTbl[idx].resetVal;
                    priority = PRIORITY_LOWEST;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].scn_rsc[idx] > gRscCtlTbl[idx].resetVal) {
                            GetScnName(i, func_name, temp_scn_name);
                            ALOGD("idx:%d, cmd:%s, comm:%s, scn_pr:%d, param:%d, pr:%d",
                                i, RscCfgTbl[idx].cmdName.c_str(), temp_scn_name, ptScnList[i].priority, ptScnList[i].scn_rsc[idx], priority);
                            if((ptScnList[i].priority == priority && ptScnList[i].scn_rsc[idx] > numToSet) || (ptScnList[i].priority < priority)) {
                                numToSet = ptScnList[i].scn_rsc[idx];
                                gRscCtlTbl[idx].curSetter = ptScnList[i].pid;
                                gRscCtlTbl[idx].curSetterName = temp_scn_name;
                                gRscCtlTbl[idx].curSetterHint = ptScnList[i].cus_lock_hint;
                                priority = ptScnList[i].priority;
                            }
                        }
                    }

                    if(numToSet == gRscCtlTbl[idx].resetVal) {
                        gRscCtlTbl[idx].curSetter = -1;
                        gRscCtlTbl[idx].curSetterName = "-1";
                        gRscCtlTbl[idx].curSetterHint = -1;
                    }

                    numofCurr = gRscCtlTbl[idx].curVal;
                    gRscCtlTbl[idx].curVal = numToSet;
                    gRscCtlTbl[idx].priority = priority;

                    if(gRscCtlTbl[idx].curVal != numofCurr) {
                        if(gRscCtlTbl[idx].curVal == gRscCtlTbl[idx].resetVal) {
                            numToSet = RscCfgTbl[idx].defaultVal;
                            result = RscCfgTbl[idx].unset_func(numToSet, (void*)&ptScnList[scenario]);
                        } else {
                            numToSet = gRscCtlTbl[idx].curVal;
                            result = RscCfgTbl[idx].set_func(numToSet, (void*)&ptScnList[scenario]);
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d pr:%d",
                            func_name, scn_name, RscCfgTbl[idx].cmdID, numToSet, gRscCtlTbl[idx].priority);
                    }
                    ALOGV("[%s] RscCfgTbl BIGGEST numToSet:%d, ret:%d", func_name, numToSet, result);
                }
            } else {
                priority = PRIORITY_LOWEST;

                for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                    if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].priority <= priority && ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal) {
                        ALOGV("idx:%d, scn_priority:%d, priority:%d", i, ptScnList[i].priority, priority);
                        priority = ptScnList[i].priority;
                    }
                }

                if(force_update) {
                    // unset original value
                    ALOGD("[%s] idx:%d, reset:%d, prev:%d, rsc:%d", func_name, idx, gRscCtlTbl[idx].resetVal, ptScnList[scenario].scn_prev_rsc[idx], ptScnList[scenario].scn_rsc[idx]);
                    if(ptScnList[scenario].scn_prev_rsc[idx] != gRscCtlTbl[idx].resetVal && \
                       ptScnList[scenario].scn_prev_rsc[idx] != ptScnList[scenario].scn_rsc[idx] ) {
                        result = RscCfgTbl[idx].unset_func(ptScnList[scenario].scn_prev_rsc[idx], (void*)&ptScnList[scenario]);
                        ALOGV("[%s] RscCfgTbl ONESHOT unset_func:%d, ret:%d", func_name, ptScnList[scenario].scn_prev_rsc[idx], result);
                    }

                    // set new value
                    if((ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal && \
                        ptScnList[scenario].scn_rsc[idx] != ptScnList[scenario].scn_prev_rsc[idx])) {
                        if(gRscCtlTbl[idx].priority == priority) {
                            result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                            gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                            ALOGV("[%s] RscCfgTbl ONESHOT cur:%d, ret:%d", func_name, gRscCtlTbl[idx].curVal, result);
                        }
                    }
                } else if((ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)) {
                    result = RscCfgTbl[idx].unset_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    ALOGI("[%s] %s update ONESHOT(unset) cmd:%x param:%d pr:%d",
                        func_name, scn_name, RscCfgTbl[idx].cmdID, ptScnList[scenario].scn_rsc[idx], gRscCtlTbl[idx].priority);

                    gRscCtlTbl[idx].priority = priority;

                    // reset low priority scenario setting (ex: game mode)
                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].oneshotReset == 1 &&
                            ptScnList[i].priority == priority && ptScnList[i].hasReset == 0 &&
                            ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal &&
                            ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].curVal) {
                            set_fpsgo_render_pid(ptScnList[i].render_pid, (void*)&ptScnList[i]);
                            result = RscCfgTbl[idx].set_func(ptScnList[i].scn_rsc[idx], (void*)&ptScnList[i]);
                            gRscCtlTbl[idx].curVal = ptScnList[i].scn_rsc[idx];
                            ptScnList[i].hasReset = 1;
                            ALOGI("[PR] %s update ONESHOT(set) cmd:%x param:%d, pr:%d", scn_name, RscCfgTbl[idx].cmdID, ptScnList[i].scn_rsc[idx], gRscCtlTbl[idx].priority);
                        }
                    }

                    // reset scenario render pid
                    set_fpsgo_render_pid(ptScnList[scenario].render_pid, (void*)&ptScnList[scenario]);
                }
            }
        }
    }

    recordCPUFreqInfo(scenario);

    return 0;
}

int perfScnDumpAll(int cmd)
{
    int i, j, idx;
    char scn_name[PACK_NAME_MAX];

    ALOGI("perfScnDumpAll cmd:%x", cmd);
    ALOGI("========================");

    switch(cmd) {
    case DUMP_DEFAULT:
        // check all user
        for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
            if (ptScnList[i].scn_state == STATE_ON) {
                GetScnName(i, "perfScnDumpAll", scn_name);

                for(int idx = 0; idx < FIELD_SIZE; idx++) {
                    //if(tConTable[idx].entry.length() == 0)
                    //    break;

                    if(tConTable[idx].isValid == -1)
                        continue;

                    if(ptScnList[i].scn_param[idx] != tConTable[idx].resetVal)
                        ALOGI("%s cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[i].scn_param[idx]);
                    }
                /*Rescontable*/
                for(int idx = 0; idx < gRscCtlTblLen; idx++) {
                    if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                        break;

                    if(gRscCtlTbl[idx].isValid != 1)
                        continue;

                    if(ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)
                        ALOGI("%s cmd:%x param:%d", scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                }
            }
        }
        break;

    case DUMP_ALL_USER:
        // check all perflock user
        for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
            if (ptScnList[i].pid != -1) {
                GetScnName(i, "perfScnDumpAll", scn_name);
                ALOGI("[perfScnDumpAll] scn:%d user_name:%s pid:%d tid:%d state:%d type:%d hdl:%d hint:%d duration:%d, ts: %s",
                    i, ptScnList[i].comm, ptScnList[i].pid, ptScnList[i].tid, ptScnList[i].scn_state,
                    ptScnList[i].scn_type, ptScnList[i].handle_idx, ptScnList[i].cus_lock_hint, ptScnList[i].lock_duration, ptScnList[i].timestamp);

                for(int idx = 0; idx < FIELD_SIZE; idx++) {
                    if(tConTable[idx].isValid == -1)
                        continue;

                    if(ptScnList[i].scn_param[idx] != tConTable[idx].resetVal)
                        ALOGI("%s - cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[i].scn_param[idx]);
                }
                /*Rescontable*/
                for(int idx = 0; idx < gRscCtlTblLen; idx++) {
                    if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                        break;

                    if(gRscCtlTbl[idx].isValid != 1)
                        continue;

                    if(ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)
                        ALOGI("%s - cmd:%x param:%d", scn_name, RscCfgTbl[idx].cmdID, ptScnList[i].scn_rsc[idx]);
                }

                std::this_thread::sleep_for(std::chrono::nanoseconds(500000));
            }
        }
        break;

    case DUMP_APP_CFG:
        ALOGI("nPackNum:%d", nPackNum);
        for (i = SCN_APP_RUN_BASE; i < SCN_APP_RUN_BASE + nPackNum; i++) {
            ALOGI("pack:%s, act:%s, fps:%s", ptScnList[i].pack_name, ptScnList[i].act_name, ptScnList[i].fps);

            for(int idx = 0; idx < FIELD_SIZE; idx++) {
                //if(tConTable[idx].entry.length() == 0)
                //    break;

                if(tConTable[idx].isValid == -1)
                    continue;

                if(ptScnList[i].scn_param[idx] != tConTable[idx].resetVal)
                    ALOGI("   cmd:%x, param:%d", tConTable[idx].cmdID, ptScnList[i].scn_param[idx]);
            }
            /*Rescontable*/
            for(int idx = 0; idx < gRscCtlTblLen; idx++) {
                if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                    break;

                if(gRscCtlTbl[idx].isValid != 1)
                    continue;

                if(ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)
                    ALOGI("   cmd:%x param:%d", RscCfgTbl[idx].cmdID, ptScnList[i].scn_rsc[idx]);
            }
        }
        break;

    default:
        // check all user
        for (i = 0; i < SCN_APP_RUN_BASE+nPackNum; i++) {
            if (ptScnList[i].scn_state == STATE_ON) {
                GetScnName(i, "perfScnDumpAll", scn_name);
                if (cmd == PERF_RES_POWERHAL_SCREEN_OFF_STATE) {
                    ALOGI("%s - SCREEN_OFF_STAT param:%d", scn_name, ptScnList[i].screen_off_action);
                }
                else if (cmd == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_COLD) {
                    ALOGI("%s - WHITELIST_APP_LAUNCH_TIME_COLD param:%d", scn_name, ptScnList[i].launch_time_cold);
                }
                else if (cmd == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_WARM) {
                    ALOGI("%s - WHITELIST_APP_LAUNCH_TIME_COLD param:%d", scn_name, ptScnList[i].launch_time_warm);
                }
                for(int idx = 0; idx < FIELD_SIZE; idx++) {
                    if(cmd == tConTable[idx].cmdID) {
                        ALOGI("%s - %s param:%d", scn_name, tConTable[idx].cmdName.c_str(), ptScnList[i].scn_param[idx]);
                    }
                }
                for(int idx = 0; idx < gRscCtlTblLen; idx++) {
                    if(cmd == RscCfgTbl[idx].cmdID) {
                        ALOGI("%s - %s param:%d", scn_name, RscCfgTbl[idx].cmdName.c_str(), ptScnList[i].scn_rsc[idx]);
                    }
                }
            }
        }
        break;
    }

    ALOGI("========================");
    return 0;
}


/*
    reset_all == 1 => reset all
    reset_all == 0 => reset resource only
 */
void resetScenario(int handle, int reset_all)
{
    LOG_V("h:%d, reset:%d", handle, reset_all);
    int i;
    if (reset_all) {
        ptScnList[handle].pack_name[0]      = '\0';
        ptScnList[handle].handle_idx        = -1;
        ptScnList[handle].scn_type          = -1;
        ptScnList[handle].scn_state         = STATE_OFF;
        ptScnList[handle].pid               = ptScnList[handle].tid = -1;
        strncpy(ptScnList[handle].comm, "   ", COMM_NAME_SIZE-1);
        ptScnList[handle].lock_rsc_size = 0;
        //memset(ptScnList[handle].lock_rsc_list, 0, sizeof(int)*sizeof(ptScnList[handle].lock_rsc_list));
        ptScnList[handle].lock_duration = 0;
        ptScnList[handle].launch_time_cold = 0;
        ptScnList[handle].launch_time_warm = 0;
        ptScnList[handle].act_switch_time = 0;
        ptScnList[handle].hint_hold_time = 0;
        ptScnList[handle].ext_hint = 0;
        ptScnList[handle].ext_hint_hold_time = 0;
        ptScnList[handle].cus_lock_hint = -1;
        ptScnList[handle].priority = NORMAL_PRIORITY;
        ptScnList[handle].oneshotReset = 0;
        strncpy(ptScnList[handle].timestamp, "\0", TIMESTAMP_MAX-1);
    }
    ptScnList[handle].screen_off_action = MTKPOWER_SCREEN_OFF_DISABLE;
    ptScnList[handle].render_pid = -1;

    for (i = 0; i < FIELD_SIZE; i++) {
        ptScnList[handle].scn_param[i] = tConTable[i].resetVal;
    }

    for (i = 0; i < gRscCtlTblLen; i++) {
        if (reset_all)
             ptScnList[handle].scn_prev_rsc[i] = gRscCtlTbl[i].resetVal;
        else // backup setting
             ptScnList[handle].scn_prev_rsc[i] = ptScnList[handle].scn_rsc[i];

        ptScnList[handle].scn_rsc[i] = gRscCtlTbl[i].resetVal;
    }
}

int cmdSetting(int icmd, char *scmd, tScnNode *scenario, int param_1, int *rsc_id)
{
    int i = 0, ret = 0;

    if ((icmd < 0) && !scmd) {
        ALOGD("cmdSetting - scmd is NULL");
        return -1;
    }

    if (((icmd == -1) && !strcmp(scmd, "PERF_RES_FPS_FPSGO_RENDER_PID")) ||
            icmd == PERF_RES_FPS_FPSGO_RENDER_PID) {
        scenario->render_pid = param_1;
        icmd = PERF_RES_FPS_FPSGO_RENDER_PID;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_SCREEN_OFF_STATE")) ||
            icmd == PERF_RES_POWERHAL_SCREEN_OFF_STATE) {
            scenario->screen_off_action = param_1;
            icmd = PERF_RES_POWERHAL_SCREEN_OFF_STATE;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_COLD")) ||
            icmd == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_COLD) {
        scenario->launch_time_cold = (param_1 >= 0) ? param_1 : -1;
        icmd = PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_COLD;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_WARM")) ||
            icmd == PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_WARM) {
        scenario->launch_time_warm = (param_1 >= 0) ? param_1 : -1;
        icmd = PERF_RES_POWERHAL_WHITELIST_APP_LAUNCH_TIME_WARM;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_WHITELIST_ACT_SWITCH_TIME")) ||
            icmd == PERF_RES_POWERHAL_WHITELIST_ACT_SWITCH_TIME) {
        scenario->act_switch_time = (param_1 >= 0) ? param_1 : -1;
        icmd = PERF_RES_POWERHAL_WHITELIST_ACT_SWITCH_TIME;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWER_HINT_EXT_HINT")) ||
            icmd == PERF_RES_POWER_HINT_EXT_HINT) {
        scenario->ext_hint = (param_1 > 0) ? param_1 : 0;
        icmd = PERF_RES_POWER_HINT_EXT_HINT;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWER_HINT_EXT_HINT_HOLD_TIME")) ||
            icmd == PERF_RES_POWER_HINT_EXT_HINT_HOLD_TIME) {
        scenario->ext_hint_hold_time = (param_1 > 0) ? param_1 : 0;
        icmd = PERF_RES_POWER_HINT_EXT_HINT_HOLD_TIME;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWER_HINT_EXT_HINT_FOR_GAME")) ||
            icmd == PERF_RES_POWER_HINT_EXT_HINT_FOR_GAME) {
        scenario->ext_hint = (param_1 > 0) ? param_1 : 0;
        icmd = PERF_RES_POWER_HINT_EXT_HINT_FOR_GAME;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWER_HINT_HOLD_TIME")) ||
            icmd == PERF_RES_POWER_HINT_HOLD_TIME) {
        scenario->hint_hold_time = (param_1 > 0) ? param_1 : 0;
        icmd = PERF_RES_POWER_HINT_HOLD_TIME;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWER_HINT_INSTALL_MAX_DURATION")) ||
            icmd == PERF_RES_POWER_HINT_INSTALL_MAX_DURATION) {
        install_max_duration = (param_1 > 0) ? param_1 : 0;
        icmd = PERF_RES_POWER_HINT_INSTALL_MAX_DURATION;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_PRIORITY")) ||
            icmd == PERF_RES_POWERHAL_PRIORITY) {
        scenario->priority = param_1;
        icmd = PERF_RES_POWERHAL_PRIORITY;
        if(param_1 == MODE_PRIORITY) {
            scenario->oneshotReset = 1;
        }
    }else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_ONESHOT_RESET")) ||
            icmd == PERF_RES_POWERHAL_ONESHOT_RESET) {
        scenario->oneshotReset = 1;
        icmd = PERF_RES_POWERHAL_ONESHOT_RESET;
    }
    else {
        ret = -1;
    }

    if (ret == 0) {
        if (rsc_id != NULL)
            *rsc_id = icmd;
        return ret;
    }

    for(int idx = 0; idx < FIELD_SIZE; idx++) {
        if(((icmd == -1) && !strcmp(scmd, tConTable[idx].cmdName.c_str())) ||
                icmd == tConTable[idx].cmdID) {
            ALOGD("cmdSetting tConTable cmdID:%x param_1:%d ,maxVal:%d ,minVal:%d", tConTable[idx].cmdID, param_1, tConTable[idx].maxVal,
                  tConTable[idx].minVal);
            if(param_1 >= tConTable[idx].minVal && param_1 <= tConTable[idx].maxVal)
                scenario->scn_param[idx] = param_1;
            else {
                ALOGE("input parameter exceed reasonable range %x %d %d %d", tConTable[idx].cmdID, param_1, tConTable[idx].maxVal,
                        tConTable[idx].minVal);
                if(param_1 < tConTable[idx].minVal)
                    scenario->scn_param[idx] = tConTable[idx].minVal;
                else if(param_1 > tConTable[idx].maxVal)
                    scenario->scn_param[idx] = tConTable[idx].maxVal;
            }

            ret = 0;
            icmd = tConTable[idx].cmdID;
        }
    }

    for(int idx = 0; idx < gRscCtlTblLen; idx++) {
        if(((icmd == -1) && !strcmp(scmd, RscCfgTbl[idx].cmdName.c_str())) ||
                icmd == RscCfgTbl[idx].cmdID) {
            ALOGD("cmdSetting RscCfgTbl cmdID:%x param_1:%d ,maxVal:%d ,minVal:%d", RscCfgTbl[idx].cmdID, param_1, RscCfgTbl[idx].maxVal,
                  RscCfgTbl[idx].minVal);
            if(param_1 >= RscCfgTbl[idx].minVal && param_1 <= RscCfgTbl[idx].maxVal)
                scenario->scn_rsc[idx] = param_1;
            else {
                ALOGE("input parameter exceed reasonable range %x %d %d %d", RscCfgTbl[idx].cmdID, param_1, RscCfgTbl[idx].maxVal,
                        RscCfgTbl[idx].minVal);
                if(param_1 < RscCfgTbl[idx].minVal)
                    scenario->scn_rsc[idx] = RscCfgTbl[idx].minVal;
                else if(param_1 > RscCfgTbl[idx].maxVal)
                    scenario->scn_rsc[idx] = RscCfgTbl[idx].maxVal;
            }

            ret = 0;
            icmd = RscCfgTbl[idx].cmdID;
        }
    }

    if (ret == -1)
        ALOGI("cmdSetting - unknown cmd:%x, scmd:%s ,param_1:%d", icmd, scmd, param_1);

    if (rsc_id != NULL)
        *rsc_id = icmd;

    return ret;
}

void Scn_cmdSetting(char *cmd, int scn, int param_1)
{
    int rsc = 0, size = 0;
    tScnNode tmp;
    cmdSetting(-1, cmd, &tmp, param_1, &rsc);
    LOG_D("scn:%d, cmd:%s, param:%d, rsc:%08x", scn, cmd, param_1, rsc);

    size = getHintRscSize(scn);
    LOG_D("size:%d", size);
    if (size < MAX_ARGS_PER_REQUEST && size % 2 == 0) {
        LOG_D("cmd:%08x, %d", rsc, param_1);
        HintRscList_append(scn, rsc, param_1);
    }
}

int getForegroundInfo(char **pPackName, int *pPid, int *pUid)
{
    LOG_I("%d", foreground_app_info.uid);
    if(pPackName != NULL)
        *pPackName = foreground_app_info.packName;

    if(pPid != NULL)
        *pPid = foreground_app_info.pid;

    if(pUid != NULL)
        *pUid = foreground_app_info.uid;

    return 0;
}

int setAPIstatus(int cmd, int enable)
{
    int i;
    if (cmd < 0 || STATUS_API_COUNT <= cmd) {
        LOG_E("undefined cmd %d", cmd);
        return 0;
    }

    powerhalAPI_status[cmd] = (enable > 0) ? 1 : 0;

    if (cmd == STATUS_APPLIST_ENABLED && enable == 0)
        for(i = 0; i < nPackNum; i++)
            perfScnDisable(SCN_APP_RUN_BASE + i);

    return 1;
}

void setInteractive(int enable)
{
    LOG_I("enable: %d", enable);
    mInteractive = enable;
}

int setGameMode(int enable, void *scn)
{
    notifyMBrainGameMode(enable);
    mode_status[GAME_MODE] = enable;
    LOG_I("enable: %d", mode_status[GAME_MODE]);

    return 1;
}

int getModeStatus(int mode_id)
{
    if (0 <= mode_id && mode_id < MODE_COUNT) {
        LOG_I("mode_id:%d, enabled:%d", mode_id, mode_status[mode_id]);
        return mode_status[mode_id];
    } else {
        LOG_E("undefined mode_id: %d", mode_id);
        return -1;
    }
}

int getGameModeStatus()
{
    LOG_E("%d", mode_status[GAME_MODE]);
    return mode_status[GAME_MODE];
}

int queryInteractiveState()
{
    LOG_D("%d", mInteractive);
    return mInteractive;
}

int setInteractiveControl(int data)
{
    mInteractiveControl = data;
    LOG_D("[SET] interactive control: %d", mInteractiveControl);
    return 0;
}

int queryInteractiveControl()
{
    LOG_D("[GET] interactive control: %d", mInteractiveControl);
    return mInteractiveControl;
}

int queryAPIstatus(int cmd)
{
    if (cmd < 0 || STATUS_API_COUNT <= cmd) {
        LOG_E("undefined cmd %d", cmd);
        return 0;
    }

    return powerhalAPI_status[cmd];
}

xml_activity* get_foreground_app_info()
{
    return &foreground_app_info;
}

static int allocNewHandleNum(void)
{
    user_handle_now = (user_handle_now + 1) % HANDLE_RAND_MAX;
    if (user_handle_now == 0)
        user_handle_now++;

    return user_handle_now;
}

static int findHandleToIndex(int handle)
{
    int i, idx = -1;

    if (handle <= 0)
        return -1;

    for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
        if((ptScnList[i].scn_type == SCN_USER_HINT || ptScnList[i].scn_type == SCN_CUS_POWER_HINT || \
            ptScnList[i].scn_type == SCN_PERF_LOCK_HINT) && (handle == ptScnList[i].handle_idx)) {
            idx = i;
            ALOGD("findHandleIndex find match handle- handle:%d idx:%d", handle, idx);
            break;
        }
    }
    return idx;
}

static int powerRscCtrl(int cmd, int id)
{
    int conIdx = -1, rscIdx = -1, value = -1, idx;

    /* find index */
    for(idx = 0; idx < FIELD_SIZE; idx++) {
        if(tConTable[idx].entry.length() == 0) // no more resource
            break;

        if(cmd == MTKPOWER_CMD_GET_RES_SETTING && tConTable[idx].isValid == -1) // invalid resource
            continue;

        if(tConTable[idx].cmdID == id) {
            conIdx = idx;
            break;
        }
    }

    if (conIdx == -1 && cmd != MTKPOWER_CMD_GET_RES_SETTING) {
        for(idx = 0; idx < gRscCtlTblLen; idx++) {
            if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                break;

            //if(gRscCtlTbl[idx].isValid != 1)
            //    continue;

            if(RscCfgTbl[idx].cmdID == id) {
                rscIdx = idx;
                break;
            }
        }
    }

    switch(cmd) {

    case MTKPOWER_CMD_GET_RES_SETTING:
        if (conIdx != -1)
            value = get_int_value(tConTable[idx].entry.c_str());
        break;

    case MTKPOWER_CMD_GET_RES_CTRL_ON:
        if (conIdx != -1) {
            tConTable[idx].isValid = 0;
            value = 0;
        } else if (rscIdx != -1) {
            gRscCtlTbl[idx].isValid = 1;
            value = 0;
        }
        break;

    case MTKPOWER_CMD_GET_RES_CTRL_OFF:
        if (conIdx != -1) {
            tConTable[idx].isValid = -1;
            value = 0;
        } else if (rscIdx != -1) {
            gRscCtlTbl[idx].isValid = 0;
            value = 0;
        }
        break;

    default:
        break;
    }

    ALOGD("powerRscCtrl cmd:%d id:%x, conIdx:%d, rscIdx:%d", cmd, id, conIdx, rscIdx);
    return value;
}

static void trigger_aee_warning(const char *aee_log)
{
    nsecs_t now = systemTime();
    ALOGD("trigger_aee_warning:%s", aee_log);

#if defined(HAVE_AEE_FEATURE)
    int interval = ns2ms(now - last_aee_time);
    if (interval > 600000 || interval < 0 || last_aee_time == 0)
        aee_system_warning("powerhal", NULL, DB_OPT_DEFAULT, aee_log);
    else
        ALOGE("trigger_aee_warning skip:%s", aee_log);
#endif
    last_aee_time = now;
}

int perfNotifyAppState(const char *packName, const char *actName, int state, int pid, int uid)
{
    LOG_I("not supported");
    return 0;
}

int perfUserScnDisableAll(void)
{
    int i;
    struct stat stat_buf;
    int exist;
    char proc_path[128];

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;
    LOG_I("");

    setInteractive(0);

    for(i=0; i<SCN_APP_RUN_BASE + nPackNum; i++) {
        if(ptScnList[i].scn_type != -1) {
            if(ptScnList[i].screen_off_action == MTKPOWER_SCREEN_OFF_ALWAYS_VALID)
                continue;

            if(ptScnList[i].scn_state == STATE_ON && ptScnList[i].screen_off_action != MTKPOWER_SCREEN_OFF_ENABLE) {
                LOG_I("perfUserScnDisableAll, h:%d, s:%d, a:%d", i, ptScnList[i].scn_state, ptScnList[i].screen_off_action);
                perfScnDisable(i);
                if(ptScnList[i].screen_off_action == MTKPOWER_SCREEN_OFF_WAIT_RESTORE)
                    ptScnList[i].scn_state = STATE_WAIT_RESTORE;
            }
            // kill handle if process is dead
            if(i >= nUserScnBase && i < SCN_APP_RUN_BASE) {
                if(ptScnList[i].scn_type == SCN_USER_HINT || ptScnList[i].scn_type == SCN_PERF_LOCK_HINT) {
                    if(snprintf(proc_path, sizeof(proc_path), "/proc/%d", ptScnList[i].pid) < 0) {
                        LOG_E("sprintf error");
                    }
                    exist = (0 == stat(proc_path, &stat_buf)) ? 1 : 0;
                    if(!exist) {
                        LOG_I("perfUserScnDisableAll, hdl:%d, pid:%d, comm:%s, is not existed", i, ptScnList[i].pid, ptScnList[i].comm);
                        perfScnDisable(i);
                        resetScenario(i, 1);
                    }
                }
            }
        }
    }

    return 0;
}

int perfUserScnRestoreAll(void)
{
    int i;

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;
    LOG_I("");

    setInteractive(1);

    // do not restore applist setting, because it is might not in the foreground
    /*
    for(i=0; i<SCN_APP_RUN_BASE + nPackNum; i++) {
        if(ptScnList[i].scn_type != -1 && ptScnList[i].scn_state == STATE_WAIT_RESTORE) {
            LOG_I("perfUserScnRestoreAll, h:%d, s:%d, a:%d", i, ptScnList[i].scn_state, ptScnList[i].screen_off_action);
            perfScnEnable(i);
        }
    }
    */
    return 0;
}

int perfUserScnCheckAll(void)
{
    int i;
    struct stat stat_buf;
    int exist, setDuration;
    char proc_path[128];

    std::unique_lock<std::mutex> lock(sMutex);
    if(!scnCheckSupport)
        return 0;

    if (!nIsReady)
        if(!init()) return 0;
    ALOGI("perfUserScnCheckAll");

    for(i=0; i<SCN_APP_RUN_BASE + nPackNum; i++) {
        if(ptScnList[i].scn_type != -1) {
            // kill handle if process is dead
            if(i >= nUserScnBase && i < SCN_APP_RUN_BASE) {
                if(ptScnList[i].screen_off_action == MTKPOWER_SCREEN_OFF_ALWAYS_VALID)
                    continue;

                if(ptScnList[i].scn_type == SCN_USER_HINT || ptScnList[i].scn_type == SCN_PERF_LOCK_HINT || ptScnList[i].scn_type == SCN_CUS_POWER_HINT) {
                    if(snprintf(proc_path, sizeof(proc_path), "/proc/%d", ptScnList[i].pid) < 0) {
                        ALOGE("sprintf error");
                    }
                    exist = (0 == stat(proc_path, &stat_buf)) ? 1 : 0;
                    setDuration = (ptScnList[i].lock_duration > 0 && ptScnList[i].lock_duration <= 30000) ? 1 : 0;
                    if(exist == 0 && setDuration == 0) {
                        ALOGI("perfUserScnDisableAll, hdl:%d, pid:%d, comm:%s, is not existed", i, ptScnList[i].pid, ptScnList[i].comm);
                        perfScnDisable(i);
                        resetScenario(i, 1);
                    }
                }
            }
        }
    }
    return 0;
}

int perfSetSysInfo(int type, const char *data)
{
    int ret = 0;
    char *pack_name = NULL, *str = NULL, *saveptr = NULL, buf[128];
    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    if (!data) {
        return -1;
    }

    switch(type) {
    case SETSYS_API_ENABLED:
        LOG_I("Enable API: %s", data);
        if (strcmp(data, "0") == 0)
            setAPIstatus(0, 1);
        else if (strcmp(data, "1") == 0)
            setAPIstatus(1, 1);
        else if (strcmp(data, "2") == 0)
            setAPIstatus(2, 1);
        else
            LOG_E("type:%d, undefined API:%s", type, data);
        break;

    case SETSYS_API_DISABLED:
        LOG_I("Disable API: %s", data);
        if (strcmp(data, "0") == 0)
            setAPIstatus(0, 0);
        else if (strcmp(data, "1") == 0)
            setAPIstatus(1, 0);
        else if (strcmp(data, "2") == 0)
            setAPIstatus(2, 0);
        else
            LOG_E("type:%d, undefined API:%s", type, data);
    break;

    case SETSYS_GAME_MODE_PID:
        LOG_D("type:%d, %s", type, data);
        game_mode_pid = strtol(data, NULL, 10);

        if ((notifyGbePid == 1) && (set_value(PATH_GBE_PID, game_mode_pid) == 0))
            notifyGbePid = 0;

        break;

    case SETSYS_VIDEO_MODE_PID:
        LOG_D("type:%d, %s", type, data);
        video_mode_pid = strtol(data, NULL, 10);
        break;

    case SETSYS_CAMERA_MODE_PID:
        LOG_D("type:%d, %s", type, data);
        camera_mode_pid = strtol(data, NULL, 10);
        break;

    case SETSYS_NETD_SET_BOOST_UID:
        LOG_D("type:%d, %s", type, data);
        foreground_app_info.uid = strtol(data, NULL, 10);
        break;

    case SETSYS_FOREGROUND_APP_PID:
        LOG_D("pid type:%d, %s", type, data);
        foreground_app_info.pid = strtol(data, NULL, 10);
        break;

    case SETSYS_MANAGEMENT_PREDICT:
        LOG_D("type:%d, %s", type, data);
        if(update_packet(data) < 0) {
            LOG_E("type:%d error", type);
        }
        notifyCmdMode(CMD_PREDICT_MODE);
        break;

    case SETSYS_MANAGEMENT_PERIODIC:
        LOG_D("type:%d, %s", type, data);
        if(update_packet(data) < 0) {
            LOG_E("type:%d error", type);
        }
        notifyCmdMode(CMD_AGGRESSIVE_MODE);
        break;

    case SETSYS_NETD_STATUS:
        ret = property_set("persist.vendor.powerhal.PERF_RES_NET_NETD_BOOST_UID", data);
        LOG_I("SETSYS_NETD_STATUS:%s ret:%d", data, ret);
        break;

    case SETSYS_PREDICT_INFO:
        LOG_D("type:%d, %s", type, data);
        strncpy(buf, data, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        str = strtok_r(buf, " ", &saveptr);
        if(str != NULL) {
            int uid = -1;
            pack_name = str;
            if((str = strtok_r(NULL, " ", &saveptr)) != NULL)
                uid = strtol(str, NULL, 10);
            LOG_D("SETSYS_PREDICT_INFO, pack:%s, uid:%d", pack_name, uid);
            notify_APPState(pack_name, uid);
        }
        break;

    case SETSYS_NETD_DUPLICATE_PACKET_LINK:
        if(strcmp(data, "DELETE_ALL") == 0) {
            if(deleteAllDupPackerLink() < 0) {
                ret = -1;
            }
        } else if(strncmp(data, "MULTI", 5) == 0) {
            if(SetDupPacketMultiLink(data) < 0) {
                ret = -1;
            }
        } else {
            if(SetOnePacketLink(data) < 0) {
                ret = -1;
            }
        }
        break;

     case SETSYS_NETD_SET_FASTPATH_BY_UID:
        LOG_I("SETSYS_NETD_SET_FASTPATH_BY_UID, data:%s", data);
        strncpy(buf, data, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        str = strtok_r(buf, " ", &saveptr);
        if(str != NULL) {
           int uid = -1;
	   char *action = str;
           if((str = strtok_r(NULL, " ", &saveptr)) != NULL)
           	uid = strtol(str, NULL, 10);
           LOG_D("SETSYS_NETD_SET_FASTPATH_BY_UID, action:%s, uid:%d", action, uid);
 	   if(strncmp(action, "SET", 3) == 0) {
        	sdk_netd_set_priority_by_uid(uid);
           } else if (strncmp(action, "CLEAR", 5) == 0) {
	 	sdk_netd_clear_priority_by_uid(uid);
	   } else {
		LOG_D("SETSYS_NETD_SET_FASTPATH_BY_UID, invalid action:%s, uid:%d", action, uid); 
		ret = -1;    
           } 
	} else {
	    ret = -1; 
	}
        break;

     case SETSYS_NETD_SET_FASTPATH_BY_LINKINFO:
        LOG_I("SETSYS_NETD_SET_FASTPATH_BY_LINKINFO, data:%s", data);
        strncpy(buf, data, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        str = strtok_r(buf, " ", &saveptr);
        if(str != NULL) {
           int uid = -1;
	   char *action = str;
	   char *srcip = strtok_r(NULL, " ", &saveptr);
	   char *srcport = strtok_r(NULL, " ", &saveptr);
	   char *dstip = strtok_r(NULL, " ", &saveptr);
	   char *dstport = strtok_r(NULL, " ", &saveptr);
	   char *proto = strtok_r(NULL, " ", &saveptr);
	   if (!srcip || !srcport || !dstip || !dstport || !proto) {
           	LOG_D("SETSYS_NETD_SET_FASTPATH_BY_LINKINFO, cmd invalid action:%s, srcip:%s:%s dstip:%s:%s proto:%s", 
			action, srcip, srcport, dstip, dstport, proto);
		ret= -1;
		break;
	    }

           LOG_D("SETSYS_NETD_SET_FASTPATH_BY_LINKINFO, action:%s, linkinfo:%s", action, saveptr);
 	   if(strncmp(action, "SET", 3) == 0) {
        	sdk_netd_set_priority_by_linkinfo(srcip, srcport, dstip, dstport, proto);
           } else if (strncmp(action, "CLEAR", 5) == 0) {
	 	sdk_netd_clear_priority_by_linkinfo(srcip, srcport, dstip, dstport, proto);
	   } else {
		LOG_D("SETSYS_NETD_SET_FASTPATH_BY_LINKINFO, invalid action:%s, linkinfo:%s", action, saveptr); 
		ret = -1;    
           } 
	} else {
	    ret = -1; 
	}
        break;

     case SETSYS_NETD_CLEAR_FASTPATH_RULES:
        LOG_I("SETSYS_NETD_CLAAR_FASTPATH_RULES, data:%s", data);
        if(strncmp(data, "UID", 3) == 0) {
        	sdk_netd_clearall(1);
        } else if (strncmp(data, "LINKINFO", 8) == 0) {
	 	sdk_netd_clearall(2);
	} else if (strncmp(data, "ALL", 3) == 0) {
	 	sdk_netd_clearall(3);
	} else {
		LOG_D("SETSYS_NETD_SET_FASTPATH_BY_LINKINFO, invalid input parameter"); 
		ret = -1;    
         } 
        break;

     case SETSYS_NETD_BOOSTER_CONFIG:
        LOG_I("[Booster]: SETSYS_NETD_BOOSTER_CONFIG, data:%s", data);
        ret = booster_netd_process_request(data);
        break;

     case SETSYS_RELOAD_WHITELIST:
        break;

     case SETSYS_POWERHAL_UNIT_TEST:
         LOG_I("SETSYS_UNIT_TEST, data:%s", data);
         if(strcmp(data, "testRscTable") == 0)
             testRscTable();
         else
            ret = -1;
         break;

    case SETSYS_SPORTS_APK:
    case SETSYS_FOREGROUND_SPORTS:
    case SETSYS_INTERNET_STATUS:
        LOG_E("not support type:%d", type);
        break;
    case SETSYS_SCN_CHECK_SUPPORT:
        LOG_D("type:%d, %s", type, data);
        scnCheckSupport = strtol(data, NULL, 10);
        break;

    case SETSYS_NOTIFY_SMART_LAUNCH_ENGINE:
        LOG_D("SETSYS_NOTIFY_SMART_LAUNCH_ENGINE type:%d, %s", type, data);
        notifySmartLaunchEngine(data);
        break;

    case SETSYS_INTERACTIVE_CONTROL:
        LOG_D("SETSYS_INTERACTIVE_CONTROL type:%d, %s", type, data);
        ret = setInteractiveControl(strtol(data, NULL, 10));
        break;

    default:
        LOG_E("unknown type");
        break;
    }


    return ret;
}

static sp<IMtkPowerCallback> gCallback;

typedef struct tEaraCallback {
    int scn_set;
    int last_sent_scn;
    sp<IMtkPowerCallback> callback;
} tEaraCallback;

using std::vector;

static vector<tEaraCallback> vEaraCallback;
static pthread_mutex_t vec_mutex = PTHREAD_MUTEX_INITIALIZER;

static void processReturn(const Return<void> &ret, const char* functionName) {
    if(!ret.isOk()) {
        LOG_I("[%s()] failed: PowerHAL gCallback not available", functionName);
        gCallback = nullptr;

    }
}


int perfSetScnUpdateCallback(int scenario_set, const sp<IMtkPowerCallback> &callback)
{

    int i;
    tEaraCallback tCb;

    pthread_mutex_lock(&vec_mutex);
    for (i=0; i<vEaraCallback.size(); i++) {
        if (vEaraCallback[i].scn_set == scenario_set && vEaraCallback[i].callback == callback) {
            pthread_mutex_unlock(&vec_mutex);
            return 0;
        }
    }

    LOG_I("[%s]set scn:%x", __FUNCTION__, scenario_set);
    tCb.scn_set = scenario_set;
    tCb.last_sent_scn = -1;
    tCb.callback = callback;

    vEaraCallback.push_back(tCb);
    pthread_mutex_unlock(&vec_mutex);

    return 0;
}

//int invokeCallback(int scn, int data)
int invokeScnUpdateCallback(int scn, int data)
{

    int i;
    Return<void> ret;
    //static int last_scn = SCN_UNDEFINE;
    vector<int> vInvalidCb;

    pthread_mutex_lock(&vec_mutex);
    LOG_I("[%s]scn:%x , vEaraCallback.size(%d)", __FUNCTION__, scn, vEaraCallback.size());

    for (i=0; i<vEaraCallback.size(); i++) {
        //bool sent_cb = false;

        if (vEaraCallback[i].scn_set == scn) {
            LOG_I("[%s] i:%d, scn is set %x", __FUNCTION__, i, scn);
            if (vEaraCallback[i].callback != nullptr)
                //ret = vEaraCallback[i].callback->mtkPowerHint(hint, data);
                ret = vEaraCallback[i].callback->notifyScnUpdate(scn, data);
            else
                continue;
            if (!ret.isOk()) {
                vEaraCallback[i].callback = nullptr;
                vInvalidCb.insert(vInvalidCb.begin(), i);
            }
            //vEaraCallback[i].last_sent_scn = scn;
            //sent_cb = true;
        }

    }

    /* clear invalid callback */
    if(!vInvalidCb.empty()) {
        for (i=0; i<vInvalidCb.size(); i++) {
            if(vInvalidCb[i] < vEaraCallback.size() && vInvalidCb[i] >= 0) {
                LOG_I("[%s]invalid callback:%d", __FUNCTION__, vInvalidCb[i]);
                vEaraCallback.erase(vEaraCallback.begin() + vInvalidCb[i]);
            }else{
                LOG_E("[%s] Invalid index detected: %d", __FUNCTION__, vInvalidCb[i]);
                break;
            }
        }
    }

    pthread_mutex_unlock(&vec_mutex);

    //last_scn = scn;
    return 0;
}

int perfDumpAll(int cmd)
{
    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;
    return perfScnDumpAll(cmd);
}

int perfUserGetCapability(int cmd, int id)
{
    int value = -1, idx = -1, i = 0;
    nsecs_t now = 0;
    int interval = -1;

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    //ALOGD("perfUserGetCapability cmd:%d, value:%x,%d", cmd, id, id);

    //if (cmd != MTKPOWER_CMD_GET_DEBUG_SET_LVL && (id >= nClusterNum || id < 0))
    //    return value;

    switch(cmd) {
    case MTKPOWER_CMD_GET_CLUSTER_NUM:
        value = nClusterNum;
        LOG_D("GET_CLUSTER_NUM cmd:%d, id:%d data:%d", cmd, id, value);
        break;

    case MTKPOWER_CMD_GET_CLUSTER_CPU_NUM:
        if(id >= 0 && id <= nClusterNum)
            value = ptClusterTbl[id].cpuNum;
        LOG_D("GET_CLUSTER_CPU_NUM cmd:%d, id:%d data:%d", cmd, id, value);
        break;

    case MTKPOWER_CMD_GET_CLUSTER_CPU_FREQ_MIN:
        if(id >= 0 && id <= nClusterNum)
            value = ptClusterTbl[id].freqMin;
        LOG_D("GET_CLUSTER_CPU_FREQ_MIN cmd:%d, id:%d data:%d", cmd, id, value);
        break;

    case MTKPOWER_CMD_GET_CLUSTER_CPU_FREQ_MAX:
        if(id >= 0 && id <= nClusterNum)
           value = ptClusterTbl[id].freqMax;
        LOG_D("GET_CLUSTER_CPU_FREQ_MAX cmd:%d, id:%d data:%d", cmd, id, value);
        break;

    case MTKPOWER_CMD_GET_FOREGROUND_PID:
        value = foreground_app_info.pid;
        LOG_D("GET_FOREGROUND_PID cmd:%d, data:%d", cmd, value);
        break;

    case MTKPOWER_CMD_GET_FOREGROUND_UID:
        value = foreground_app_info.uid;
        break;

    case MTKPOWER_CMD_GET_POWER_HDL_HOLD_TIME:
        idx = findHandleToIndex(id);
        if(checkSuccess(idx))
            value = ptScnList[idx].hint_hold_time;
        if (value)
            LOG_D("MTKPOWER_CMD_GET_POWER_HDL_HOLD_TIME cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT:
        idx = findHandleToIndex(id);
        if(checkSuccess(idx))
            value = ptScnList[idx].ext_hint;
        if (value)
            LOG_D("MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT cmd:%d, data:%d", cmd, value);
        break;

    case MTKPOWER_CMD_GET_GPU_FREQ_COUNT:
        value = nGpuFreqCount;
        LOG_D("GET_GPU_FREQ_COUNT cmd:%d, data:%d", cmd, value);
        break;

    case MTKPOWER_CMD_GET_RILD_CAP:
        value = query_capability(id, "GLEN");
        LOG_D("GET_RILD_CAP cmd:%d, data:%d", cmd, value);
        break;

    case MTKPOWER_CMD_GET_TIME_TO_LAST_TOUCH:
        now = systemTime();
        interval = ns2ms(now - last_touch_time);
        if (interval > 100000 || interval < 0)
            value = 100000;
        else
            value = interval;
        LOG_D("GET_TIME_TO_LAST_TOUCH now:%lld, value:%lld interval:%d",
            (long long)now, (long long)last_touch_time, interval);
        break;

    case MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT_HOLD_TIME:
        idx = findHandleToIndex(id);
        if(checkSuccess(idx))
            value = ptScnList[idx].ext_hint_hold_time;
        if (value)
            LOG_D("MTKPOWER_CMD_GET_POWER_HDL_EXT_HINT_HOLD_TIME cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_INSTALL_MAX_DURATION:
        value = install_max_duration;
        if (value)
            LOG_I("MTKPOWER_CMD_GET_INSTALL_MAX_DURATION cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_LAUNCH_TIME_COLD:
        value = fg_launch_time_cold;
        if (value)
            LOG_I("GET_LAUNCH_TIME_COLD cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_LAUNCH_TIME_WARM:
        value = fg_launch_time_warm;
        if (value)
            LOG_I("GET_LAUNCH_TIME_WARM cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_ACT_SWITCH_TIME:
        value = fg_act_switch_time;
        if (value)
            LOG_I("MTKPOWER_CMD_GET_ACT_SWITCH_TIME cmd:%d, time:%d ms", cmd, value);
        break;

    case MTKPOWER_CMD_GET_PROCESS_CREATE_STATUS:
        value = 0;
        for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
            if (ptScnList[i].cus_lock_hint == MTKPOWER_HINT_PROCESS_CREATE) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    value = 1;
                }
            }
        }
        break;

    case MTKPOWER_CMD_GET_POWER_SCN_TYPE:
        idx = findHandleToIndex(id);
        if (idx != -1)
            value = ptScnList[idx].scn_type;
        else
            value = -1;
        LOG_D("GET_POWER_SCN_TYPE cmd:%d, scn:%d, type:%d", cmd, id, value);
        break;

    case MTKPOWER_CMD_GET_CPU_TOPOLOGY:
        //value = ptClusterTbl[id].cpuNum;
        break;

    case MTKPOWER_CMD_GET_RES_SETTING:
        value = powerRscCtrl(MTKPOWER_CMD_GET_RES_SETTING, id);
        break;

    case MTKPOWER_CMD_GET_DEBUG_DUMP_ALL:
        value = perfScnDumpAll(DUMP_DEFAULT);
        break;

    case MTKPOWER_CMD_GET_DEBUG_DUMP_APP_CFG:
        value = perfScnDumpAll(DUMP_APP_CFG);
        break;

    case MTKPOWER_CMD_GET_RES_CTRL_ON:
        value = powerRscCtrl(MTKPOWER_CMD_GET_RES_CTRL_ON, id);
        break;

    case MTKPOWER_CMD_GET_RES_CTRL_OFF:
        value = powerRscCtrl(MTKPOWER_CMD_GET_RES_CTRL_OFF, id);
        break;

    case MTKPOWER_CMD_GET_INTERACTIVE_STATE:
        value = queryInteractiveState();
        break;

    case MTKPOWER_CMD_GET_POWERHAL_MODE_STATUS:
        value = getModeStatus(id);
        break;

    case MTKPOWER_CMD_GET_API_STATUS:
        value = queryAPIstatus(id);
        LOG_I("API: %d, enabled: %d", id, value);
        break;

    case MTKPOWER_CMD_GET_TOUCH_BOOST_STATUS:
        break;

    case PERF_RES_POWERHAL_TEST_CMD:
        value = testSetting;
        break;

    case MTKPOWER_CMD_GET_SCN_CHECK_SUPPORT:
        value = scnCheckSupport;
        break;

    case MTKPOWER_CMD_GET_SCN_PID:
        if(id > 0) {
            idx = findHandleToIndex(id);
        } else {
            LOG_E("error hdl: %d", id);
            break;
        }

        if(idx >= 0 && idx < REG_SCN_MAX) {
            value = ptScnList[idx].pid;
        } else
            LOG_E("error idx: %d", idx);

        break;

    case MTKPOWER_CMD_GET_ADPF_CPU_LOAD_UP_BOOST_DURATION:
        value = get_cpu_load_up_boost_duration();
        break;

    case MTKPOWER_CMD_GET_ADPF_CPU_LOAD_SPIKE_BOOST_DURATION:
        value = get_cpu_load_spike_boost_duration();
        break;

    case MTKPOWER_CMD_GET_ADPF_CPU_LOAD_UP_BOOST_SCALE:
        value = get_cpu_load_up_boost_scale();
        break;

    case MTKPOWER_CMD_GET_ADPF_CPU_LOAD_SPIKE_BOOST_SCALE:
        value = get_cpu_load_spike_boost_scale();
        break;

    case MTKPOWER_CMD_GET_INTERACTIVE_CONTROL:
        value = queryInteractiveControl();
        break;

    default:
        LOG_D("error unknown cmd:%d, id:%d", cmd, id);
        break;
    }

    LOG_D("cmd:%d, value:%d", cmd, value);
    return value;
}

int perfLockCheckHdl_internal(int handle, int hint, int size, int pid, int tid, int duration, int *pIndex)
{
    int i, idx;
    char filepath[64] = "\0";
    char proc_path[128];
    int exist, setDuration;
    struct stat stat_buf;

    if (size > MAX_ARGS_PER_REQUEST || size == 0) {
        LOG_E("size:%d > MAX_ARGS_PER_REQUEST:%d", size, MAX_ARGS_PER_REQUEST);
        return 0;
    }

    /*check user have permission otherwise duration must < 30s */
    if(snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid) < 0) {
        LOG_E("sprintf error");
    }
    exist = (0 == stat(proc_path, &stat_buf)) ? 1 : 0;
    LOG_D("handle:%d pid:%d duration:%d exist:%d", handle, pid, duration, exist);

    /* duration == 0 means infinite duration */
    if(!exist && (duration == 0 || duration > USER_DURATION_MAX)) {
        LOG_E("pid:%d don't have permission !! duration:%d ms > 30s", pid, duration);
        return 0;
    }

    idx = findHandleToIndex(handle);

    if(idx == -1) {
        for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
            if(ptScnList[i].scn_type == -1) {
                handle = allocNewHandleNum();
                resetScenario(i, 1);
                if (hint != -1) {
                    ptScnList[i].scn_type = SCN_CUS_POWER_HINT;
                    ptScnList[i].cus_lock_hint = hint;
                } else {
                    ptScnList[i].scn_type = SCN_PERF_LOCK_HINT;
                }
                ptScnList[i].handle_idx = handle;
                ptScnList[i].lock_duration = duration;
                ptScnList[i].pid = pid;
                ptScnList[i].tid = tid;
                if(snprintf(filepath, sizeof(filepath), "/proc/%d/comm", pid) < 0) {
                    LOG_E("snprint error");
                }
                get_task_comm(filepath, ptScnList[i].comm);
                ptScnList[i].scn_state = STATE_OFF;
                idx = i;
                LOG_D("register handle - handle:%d idx:%d", handle, idx);
                break;
            }
        }

        // Check again by perfUserScnCheckAll() whether really no more handle
        if(idx == -1) {
            ALOGI("no more handle, check all handle status");
            for(i=0; i<SCN_APP_RUN_BASE + nPackNum; i++) {
                if(ptScnList[i].scn_type != -1) {
                    // kill handle if process is dead
                    if(i >= nUserScnBase && i < SCN_APP_RUN_BASE) {
                        if(ptScnList[i].screen_off_action == MTKPOWER_SCREEN_OFF_ALWAYS_VALID)
                            continue;

                        if(ptScnList[i].scn_type == SCN_USER_HINT || ptScnList[i].scn_type == SCN_PERF_LOCK_HINT || ptScnList[i].scn_type == SCN_CUS_POWER_HINT) {
                            if(snprintf(proc_path, sizeof(proc_path), "/proc/%d", ptScnList[i].pid) < 0) {
                                ALOGE("sprintf error");
                            }
                            exist = (0 == stat(proc_path, &stat_buf)) ? 1 : 0;
                            setDuration = (ptScnList[i].lock_duration > 0 && ptScnList[i].lock_duration <= 30000) ? 1 : 0;
                            if(exist == 0 && setDuration == 0) {
                                ALOGI("perfUserScnDisableAll, hdl:%d, pid:%d, comm:%s, is not existed", i, ptScnList[i].pid, ptScnList[i].comm);
                                perfScnDisable(i);
                                resetScenario(i, 1);
                            }
                        }
                    }
                }
            }

            // Check whether the empty handle
            for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
                if(ptScnList[i].scn_type == -1) {
                    handle = allocNewHandleNum();
                    resetScenario(i, 1);
                    if (hint != -1) {
                        ptScnList[i].scn_type = SCN_CUS_POWER_HINT;
                        ptScnList[i].cus_lock_hint = hint;
                    } else {
                        ptScnList[i].scn_type = SCN_PERF_LOCK_HINT;
                    }
                    ptScnList[i].handle_idx = handle;
                    ptScnList[i].lock_duration = duration;
                    ptScnList[i].pid = pid;
                    ptScnList[i].tid = tid;
                    if(snprintf(filepath, sizeof(filepath), "/proc/%d/comm", pid) < 0) {
                        LOG_E("snprint error");
                    }
                    get_task_comm(filepath, ptScnList[i].comm);
                    ptScnList[i].scn_state = STATE_OFF;
                    idx = i;
                    LOG_D("register handle - handle:%d idx:%d", handle, idx);
                    break;
                }
            }
        }

        /* no more empty slot */
        if(idx == -1) {
            LOG_E("cannot alloc handle!!! dump all user!!!");
            perfScnDumpAll(DUMP_ALL_USER);
            trigger_aee_warning("perf_lock_acq no more handle");
            return 0;
        }

    }
#if 0 // do it in perfLockAcq
    else {
        resetScenario(idx, 0); // reset resource only
        ptScnList[idx].lock_duration = duration;
    }
#endif

    *pIndex = idx;
    return handle;
}

int perfLockCheckHdl(int handle, int hint, int size, int pid, int tid, int duration)
{
    int index;

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    return perfLockCheckHdl_internal(handle, hint, size, pid, tid, duration, &index);
}

int keepTimeStamp(int idx) {
    if (idx < 0 || idx >= REG_SCN_MAX)
        return -1;

    char totalStr[TIMESTAMP_MAX], timeStr[32], msStr[4];
    int i = 2;

    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(value);
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&now_c);
    if(tm == NULL) {
        ALOGE("localtime error");
        return -1;
    }

    size_t timeStrSize = std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

    if(timeStrSize <= 0) {
        ALOGE("strftime error, size:%d", timeStrSize);
        return -1;
    }

    int ms_count = ms.count() % 1000;

    while(i >= 0) {
        msStr[i--] = '0' + ms_count%10;
        ms_count /= 10;
    }
    msStr[sizeof(msStr)-1] = '\0';

    if (snprintf(totalStr, TIMESTAMP_MAX, "%s.%s", timeStr, msStr) < 0) {
        ALOGE("snprint error");
        return -1;
    }

    strncpy(ptScnList[idx].timestamp, totalStr, TIMESTAMP_MAX);
    ptScnList[idx].timestamp[TIMESTAMP_MAX-1] = '\0';

    return 0;
}

int perfLockAcq(int *list, int handle, int size, int pid, int tid, int duration, int hint)
{
    int i, idx = -1;
    int cmd, value, ret_handle;
    int hdl_enabled = 0, rsc_modified = 0;
    int log_hasI = 1;

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    LOG_D("handle:%d size:%d duration:%d pid:%d hint:%d", handle, size, duration, pid, hint);

    if (size > MAX_ARGS_PER_REQUEST || size == 0) {
        LOG_E("size:%d > MAX_ARGS_PER_REQUEST:%d", size, MAX_ARGS_PER_REQUEST);
        return 0;
    }

    if (size % 2 == 0) {
        for (i=0; i<size; i+=2)
            LOG_D("data:0x%08x, %d", list[i], list[i+1]);
    }

    idx = findHandleToIndex(handle);

    if(keepTimeStamp(idx) != 0)
        return 0;

    if (idx == -1) {
        ret_handle = perfLockCheckHdl_internal(0, hint, size, pid, tid, duration, &idx);
        if (ret_handle <= 0)
            return 0;
    } else {
        ret_handle = handle;
        resetScenario(idx, 0); // reset resource only. Not reset in perfLockCheckHdl_internal
        ptScnList[idx].lock_duration = duration;
    }

    if (STATE_ON == ptScnList[idx].scn_state)
        hdl_enabled = 1;

    LOG_D("hdl_enabled:%d", hdl_enabled);
    /* backup resource */
    if ((ptScnList[idx].lock_rsc_list = (int*)malloc(sizeof(int) * size)) == NULL) {
        ALOGE("Cannot allocate memory for ptScnList[idx].lock_rsc_list");
        return 0;
    }

    if (size % 2 == 0) {
        for (i=0; i<size; i+=2)
            LOG_D("ptScnList data:0x%08x, %d", ptScnList[idx].lock_rsc_list[i], ptScnList[idx].lock_rsc_list[i+1]);
    }

    if(hdl_enabled == 0) {
        ptScnList[idx].lock_rsc_size = size;
        memcpy(ptScnList[idx].lock_rsc_list, list, sizeof(int) * size);
    } else {
        if (ptScnList[idx].lock_rsc_size != size || \
            memcmp(ptScnList[idx].lock_rsc_list, list, sizeof(int) * size) != 0) {
            LOG_D("rsc_modified");
            rsc_modified = 1;

            if(ptScnList[idx].lock_rsc_size != size)
                ptScnList[idx].lock_rsc_size = size;
            memcpy(ptScnList[idx].lock_rsc_list, list, sizeof(int) * size);
        }
    }

    for(i = 0; (2*i) < size; i++) {
        cmd = list[2*i];
        value = list[2*i+1];
        cmdSetting(cmd, NULL, &ptScnList[idx], value, NULL);
    }

    if(log_hasI)
        LOG_I("idx:%d hdl:%d hint:%d pid:%d tid:%d duration:%d lock_user:%s => ret_hdl:%d", idx, handle, hint, pid, (tid == 0) ? pid : tid, duration, ptScnList[idx].comm, ret_handle);
	else
        LOG_D("idx:%d hdl:%d hint:%d pid:%d duration:%d lock_user:%s => ret_hdl:%d", idx, handle, hint, pid, duration, ptScnList[idx].comm, ret_handle);

    if(hdl_enabled == 1 && rsc_modified == 1) {
        LOG_D("perflockAcq force update!!!");
        perfScnUpdate(idx, 1); // update setting
    }
    else {
        perfScnEnable(idx);
    }

    free(ptScnList[idx].lock_rsc_list);

    LOG_I("Done for hdl : %d", handle);

    return ret_handle;
}

int perfLockRel(int handle)
{
    int idx = -1;

    LOG_V("handle:%d", handle);

    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    idx = findHandleToIndex(handle);
    LOG_I("hdl:%d, idx:%d", handle, idx);
    if(idx < 0 || idx < nUserScnBase || idx >= nUserScnBase + REG_SCN_MAX)
        return -1;

    perfScnDisable(idx);

    //ptScnList[idx].lock_rsc_size = 0;

    if(ptScnList[idx].scn_state == STATE_ON)
        return -1;

    resetScenario(idx, 1);

    LOG_I("Done for hdl : %d", handle);

    return 0;
}

int perfGetHintRsc(int hint, int *size, int *rsc_list)
{
    std::unique_lock<std::mutex> lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    if (getHintRscSize(hint) < 0 || getHintRscSize(hint) > MAX_ARGS_PER_REQUEST) {
        LOG_E("Error size:%d", getHintRscSize(hint));
        return 0;
    }
    if(size != NULL) {
        *size = getHintRscSize(hint);
        if (*size <= 0)
            return 0;

        LOG_D("hint:%d %s", hint, getHintName(hint).c_str());
        for(int i = 0; i < *size; i = i+2) {
            LOG_D("cmd: %x, param: %d", getHintRscElement(hint, i), getHintRscElement(hint, i+1));
        }
        if(rsc_list != NULL) {
            if (getHintRscList(hint) != nullptr)
                memcpy(rsc_list, getHintRscList(hint), sizeof(int)*(*size));
            else {
                LOG_E("[getHintRscList] hint_id %d cannot be found in PowerScnTbl", hint);
                return 0;
            }
        }
    }
    return 0;
}

int perfCusLockHint(int hint, int duration, int pid)
{
    int i, size;
    size = ptScnList[hint].lock_rsc_size;
    LOG_I("hint:%d, dur:%d, pid:%d, size:%d", hint, duration, pid, size);

    if (size == 0 || size % 2 != 0)
        return 0;

    if (size % 2 == 0) {
        for (i=0; i<size; i+=2)
            LOG_D("data:0x%08x, %d",
            ptScnList[hint].lock_rsc_list[i], ptScnList[hint].lock_rsc_list[i+1]);
    }

    return perfLockAcq(ptScnList[hint].lock_rsc_list, 0, ptScnList[hint].lock_rsc_size, pid, 0, duration, hint);
}

//extern "C"
int perfLibpowerInit(void)
{
    std::unique_lock<std::mutex> lock(sMutex);

    if (!nIsReady)
        if(!init()) return -1;

    return 0;
}

void setCPUFreqRsc(int idx) {
    if(idx < 0 || idx >= gRscCtlTblLen) {
        ALOGE("idx error! %d", idx);
        return;
    }

    int clusterMinIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MIN_CLUSTER_0)/RSC_IDX_DIFF;
    int clusterMaxIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MAX_CLUSTER_0)/RSC_IDX_DIFF;
    int clusterHLMinIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0)/RSC_IDX_DIFF;
    int clusterHLMaxIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0)/RSC_IDX_DIFF;
    int cpuMinIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MIN_CORE_0)/RSC_IDX_DIFF;
    int cpuMaxIdx = (RscCfgTbl[idx].cmdID - PERF_RES_CPUFREQ_MAX_CORE_0)/RSC_IDX_DIFF;

    if(clusterMinIdx >= 0 && clusterMinIdx < nClusterNum) {
        soft_freq[clusterMinIdx].rscTblMinIdx = idx;
    }else if(clusterMaxIdx >= 0 && clusterMaxIdx < nClusterNum) {
        soft_freq[clusterMaxIdx].rscTblMaxIdx = idx;
    }else if(clusterHLMinIdx >= 0 && clusterHLMinIdx < nClusterNum) {
        hard_freq[clusterHLMinIdx].rscTblMinIdx = idx;
    }else if(clusterHLMaxIdx >= 0 && clusterHLMaxIdx < nClusterNum) {
        hard_freq[clusterHLMaxIdx].rscTblMaxIdx = idx;
    }else if(cpuMinIdx >= 0 && cpuMinIdx < nCpuNum) {
        soft_core_freq[cpuMinIdx].rscTblMinIdx = idx;
    }else if(cpuMaxIdx >= 0 && cpuMaxIdx < nCpuNum) {
        soft_core_freq[cpuMaxIdx].rscTblMaxIdx = idx;
    }
}

int loadRscTable(int power_on_init)
{
    int i;
    int ret;

    for(i = 0; i < gRscCtlTblLen; i++) {
        LOG_D("RscCfgTbl[%d] cmdName:%s cmdID:%x, param:%d, defaultVal:%d comp:%d maxVal:%d",
        i, RscCfgTbl[i].cmdName.c_str(), RscCfgTbl[i].cmdID, RscCfgTbl[i].defaultVal, RscCfgTbl[i].comp,
            RscCfgTbl[i].maxVal, RscCfgTbl[i].minVal);

        /*reset all Rsctable*/
        if (RscCfgTbl[i].init_func != NULL)
            ret = RscCfgTbl[i].init_func(power_on_init);

        // initial setting should be an invalid value
        if (RscCfgTbl[i].comp == SMALLEST)
            gRscCtlTbl[i].resetVal = RscCfgTbl[i].maxVal + 1;
        else if (RscCfgTbl[i].comp == BIGGEST)
            gRscCtlTbl[i].resetVal = RscCfgTbl[i].minVal - 1;
        else
            gRscCtlTbl[i].resetVal = RSC_TBL_INVALID_VALUE;

        gRscCtlTbl[i].curVal = gRscCtlTbl[i].resetVal;
        gRscCtlTbl[i].curSetter = -1;
        gRscCtlTbl[i].curSetterName = "-1";
        gRscCtlTbl[i].curSetterHint = -1;
        gRscCtlTbl[i].isValid = 1;
        gRscCtlTbl[i].log = 1;
        gRscCtlTbl[i].priority = PRIORITY_LOWEST;
        setCPUFreqRsc(i);

        LOG_D("gRscCtlTbl[%d] resetVal:%d curVal:%d isValid:%d log:%d pr:%d",
        i, gRscCtlTbl[i].resetVal, gRscCtlTbl[i].curVal, gRscCtlTbl[i].isValid, gRscCtlTbl[i].log, gRscCtlTbl[i].priority);
    }

    return 1;
}


int testRscTable(void)
{
    int i;
    int ret;

    for(i = 0; i < gRscCtlTblLen; i++) {
        LOG_I("RscCfgTbl[%d] cmdName:%s cmdID:%x, defaultVal:%d comp:%d maxVal:%d minVal:%d",
        i, RscCfgTbl[i].cmdName.c_str(), RscCfgTbl[i].cmdID, RscCfgTbl[i].defaultVal, RscCfgTbl[i].comp,
            RscCfgTbl[i].maxVal, RscCfgTbl[i].minVal);
        if (RscCfgTbl[i].set_func != NULL)
            ret = RscCfgTbl[i].set_func(RscCfgTbl[i].maxVal, (void*)&ptScnList[MTKPOWER_HINT_BASE]);
    }

    return 1;
}

int check_core_ctl_ioctl(void)
{
    if (core_ctl_dev_fd >= 0) {
        return 0;
    } else if (core_ctl_dev_fd == -1) {
        core_ctl_dev_fd = open(PATH_CORE_CTL_IOCTL, O_RDONLY);
        // file not exits
        if (core_ctl_dev_fd < 0 && errno == ENOENT)
            core_ctl_dev_fd = -2;
        // file exist, but can't open
        if (core_ctl_dev_fd == -1) {
            LOG_E("Can't open %s: %s", PATH_CORE_CTL_IOCTL, strerror(errno));
            return -1;
        }
        // file not exist
    } else if (core_ctl_dev_fd == -2) {
        //LOG_E("Can't open %s: %s", PATH_EARA_IOCTL, strerror(errno));
        return -2;
    }
    return 0;
}

void disableScenarioByHintId(int hindId)
{
    int i = 0;
    int idex = -1;
    LOG_I("Disable HindID : %d", hindId);
    for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
        if((ptScnList[i].scn_type == SCN_CUS_POWER_HINT) && (ptScnList[i].cus_lock_hint == hindId)) {
            idex = i;
            LOG_I("disableScenarioByHintId: HindID : %d", ptScnList[i].cus_lock_hint);
            break;
        }
    }
    if (idex != -1)
    {
        perfScnDisable(idex);
        resetScenario(idex, 1);
    }
    return;
}

int isPowerhalReady()
{
    return nIsReady;
}

int isPowerLibEnable()
{
    return nPowerLibEnable;
}

int get_cpu_dma_latency_value()
{
    int i;
    int value = 0;

    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_PM_QOS_CPU_DMA_LATENCY_VALUE")) {
            value = (tConTable[i].curVal == tConTable[i].resetVal) ? tConTable[i].defaultVal : tConTable[i].curVal;
        }
    }
    LOG_I("%d", value);

    return value;
}

int getCPUFreq(int cid, int opp)
{
    int cpufreq = -1;
    int idx;

    if (cid < 0 || cid >= nClusterNum) {
        LOG_E("error cid:%d, nClusterNum:%d", cid, nClusterNum);
        return -1;
    }

    if (opp < 0 || opp >= ptClusterTbl[cid].freqCount) {
        LOG_D("opp:%d, ptClusterTbl[%d].freqCount:%d", opp, cid, ptClusterTbl[cid].freqCount);
        return -1;
    }

    idx = (ptClusterTbl[cid].freqCount-1) - opp;
    cpufreq = ptClusterTbl[cid].pFreqTbl[idx];

    LOG_D("cid:%d, opp:%d => cpufreq:%d", cid, opp, cpufreq);
    return cpufreq;
}

int getCPUFreqOppCnt(int cid)
{

    if (cid < 0 || cid >= nClusterNum) {
        LOG_E("error cid:%d, nClusterNum:%d", cid, nClusterNum);
        return -1;
    }

    return ptClusterTbl[cid].freqCount -1;
}

int findClosestNumber(const int numbers[], int size, int target) {
    if (size == 0) {
        return 0;
    }

    int closest = numbers[0];
    int min_diff = abs(target - closest);

    for (int i = 0; i < size; ++i) {
        int diff = abs(target - numbers[i]);
        if (diff < min_diff) {
            min_diff = diff;
            closest = numbers[i];
        }
    }

    return closest;
}

int getCPUFreqToOpp(int cid, int freq)
{
    int opp_cnt = 0;
    int idx = 0;
    int opp = 0;
    int closestfreq = 0;

    if (cid < 0 || cid >= nClusterNum) {
        LOG_E("error cid:%d, nClusterNum:%d", cid, nClusterNum);
        return -1;
    }

    opp_cnt = ptClusterTbl[cid].freqCount;
    closestfreq = findClosestNumber(ptClusterTbl[cid].pFreqTbl, opp_cnt, freq);

    for (idx = 0; idx < opp_cnt; idx++) {
        if (ptClusterTbl[cid].pFreqTbl[idx] == closestfreq) {
            opp = idx;
        }
    }

    return opp ? opp_cnt -1 - opp : 0;
}


int get_system_mask_status(int cmd)
{
    int i, idx, result = 0;
    char scn_name[PACK_NAME_MAX];

    for (idx = 0; idx < gRscCtlTblLen; idx++)
        if (RscCfgTbl[idx].cmdID == cmd)
            break;

    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
        if (ptScnList[i].scn_state == STATE_ON && ptScnList[i].scn_rsc[idx] >= 0) {
            GetScnName(i, "GetMaskUser", scn_name);
            LOG_D("idx:%d user:%s param:%d (0x%x)", i, scn_name, ptScnList[i].scn_rsc[idx], ptScnList[i].scn_rsc[idx]);
            result = result | ptScnList[i].scn_rsc[idx];
        }
    }
    LOG_D("result: %d (0x%x)", result, result);

    return result;
}

void update_default_core_min()
{
    int i;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_CPUCORE_MIN_CLUSTER_0")) {
            default_core_min[0] = tConTable[i].defaultVal;
        } else if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_CPUCORE_MIN_CLUSTER_1")) {
            default_core_min[1] = tConTable[i].defaultVal;
        } else if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_CPUCORE_MIN_CLUSTER_2")) {
            default_core_min[2] = tConTable[i].defaultVal;
        }
    }

    if (check_core_ctl_ioctl() == 0) {
        for (i = 0; i < nClusterNum; i ++) {
            _CORE_CTL_PACKAGE msg;
            msg.cid = i;
            msg.min = default_core_min[i];
            msg.max = ptClusterTbl[i].cpuNum;;
            LOG_I("cid:%d min:%d max:%d", msg.cid, msg.min, msg.max);
            ioctl(core_ctl_dev_fd, CORE_CTL_SET_LIMIT_CPUS, &msg);
        }
    }
}

int load_mbrain_api(void)
{
    const int bind_flags = RTLD_NOW | RTLD_LOCAL;
    if (nullptr == libMBrainHandle) {
        libMBrainHandle = dlopen(g_libMBSDKvFilename, bind_flags);
        if(nullptr != libMBrainHandle) {
            notifyMbrainCpuFreqCap = reinterpret_cast<NotifyCpuFreqCapSetupHook>(dlsym(libMBrainHandle, "NotifyCpuFreqCapSetupHook"));
            if(nullptr == notifyMbrainCpuFreqCap) {
                LOG_E("notifyMbrainCpuFreqCap error: %s", dlerror());
                dlclose(libMBrainHandle);
                libMBrainHandle = nullptr;
                return -1;
            }
            notifyMBrainGameModeEnabled = reinterpret_cast<NotifyGameModeEnabledHook>(dlsym(libMBrainHandle, "NotifyGameModeEnabledHook"));
            if(nullptr == notifyMBrainGameModeEnabled) {
                LOG_E("notifyMBrainGameModeEnabled error: %s", dlerror());
                dlclose(libMBrainHandle);
                libMBrainHandle = nullptr;
                return -1;
            }
        }
        else {
            LOG_E("Can not load %s, reason: %s", g_libMBSDKvFilename, dlerror());
            return -1;
        }
    }
    LOG_I("load mbrain successfully");

    return 0;
}

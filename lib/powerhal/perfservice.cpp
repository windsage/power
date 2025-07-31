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
#define ATRACE_TAG_PERF (1<<12)
#define ATRACE_TAG ATRACE_TAG_PERF

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

#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <expat.h>

#include "mtkpower_hint.h"
#include "mtkperf_resource.h"
#include "mtkpower_types.h"
#include <utils/Timers.h>
#include "perfservice_rsccfgtbl.h"
#include "perfservice_prop.h"
#include "utility_ux.h"
#include "utility_ril.h"
#include "utility_netd.h"
#include "utility_touch.h"
#include "power_msg_handler.h"
#include "power_boot_handler.h"
#include "power_error_handler.h"
#include "perfservice_scn.h"
#include "utility_sbe_handle.h"
#include "utility_fps.h"
#include "utility_sched.h"
//SPD: add powermode by rui.zhou6 20240902 start
#include"utility_power.h"
//SPD: add powermode by rui.zhou6 20240902 end
//SPD:porting policy by sifengtian 20230525 start
#include "utility_thermal_ux.h"
//SPD:porting thermal ux policy by sifengtian 20230525 end

#if 1 //HAVE_MBRAIN
typedef int (*NotifyCpuFreqCapSetupHook)(int pid, int tid, int hdl, int duration, int hintId, int clusterId, int qosType, int freq);
typedef int (*NotifyGameModeEnabledHook)(bool enabled);
typedef int (*NotifyToCloseDBHook)();

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

#define PACK_NAME_MAX   128
#define CLASS_NAME_MAX  128
#define CUS_SCN_TABLE           "/vendor/etc/powerscntbl.xml"
#define CUS_CONFIG_TABLE        "/vendor/etc/powercontable.xml"
#define CUS_CONFIG_TABLE_T      "/vendor/etc/powercontable_t.xml"
#define COMM_NAME_SIZE  64
#define MULTI_WIN_SIZE_MAX   30
//SPD:porting thermal ux policy by sifengtian 20230525 start
#define CUS_SCN_TABLE_2         "/data/vendor/powerhal/powerscntbl.xml"
#define CUS_CONFIG_TABLE_2      "/data/vendor/powerhal/powercontable.xml"
//SPD:porting thermal ux policy by sifengtian 20230525 end

#define REG_SCN_MAX     256   // user scenario max number
#define CLUSTER_MAX     8

#define FBC_LIB_FULL_NAME  "libperfctl_vendor.so"
#define LIB_POWER_APPLIST "lib_power_applist.so"

#define CPU_CORE_MIN_RESET  (-1)
#define CPU_CORE_MAX_RESET  (0xff)
#define CPU_FREQ_MIN_RESET  (-1)
#define CPU_FREQ_MAX_RESET  (0xffffff)
#define GPU_FREQ_MIN_RESET  (-1)
#define GPU_FREQ_MAX_RESET  (-1)
//SPD:porting thermal ux policy by sifengtian 20230525 start
#define THERMAL_UX_CPU_MAX_RESET (-1)
//SPD:porting thermal ux policy by sifengtian 20230525 end

#define CORE_MAX        0xff
#define FREQ_MAX        0xffffff
#define PPM_IGNORE      (-1)

// core_ctl default setting
#define CORE_CTL_CPUCORE_MIN_CLUSTER_0 4
#define CORE_CTL_CPUCORE_MIN_CLUSTER_1 2
#define CORE_CTL_CPUCORE_MIN_CLUSTER_2 0

#define HARDLIMIT_ENABLED 1
#define GPU_HARDLIMIT_ENABLED 1

const string LESS("less");
const string MORE("more");

#define HANDLE_RAND_MAX 2147483645
#define USER_DURATION_MAX 30000 /*30s for non permission user*/

#define RSC_TBL_INVALID_VALUE (-123456)

#define ATRACE_MESSAGE_LEN 256

#define PRIORITY_HIGHEST 0
#define PRIORITY_LOWEST 5

#ifdef max
#undef max
#endif
#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a,b) (((a) < (b)) ? (a) : (b))

using namespace std;

typedef struct tClusterInfo {
    int  cpuNum;
    int  cpuFirstIndex;
    int  cpuMinNow;
    int  cpuMaxNow;
    int *pFreqTbl;
    int  freqCount;
    int  freqMin;
    int  freqMax;
    int  freqMinNow;
    int  freqMaxNow;
    int  freqHardMinNow;
    int  freqHardMaxNow;
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    int  freqThermalUxNow;
    //SPD:porting thermal ux policy by sifengtian 20230525 end
    char *sysfsMinPath;
    char *sysfsMaxPath;
} tClusterInfo;

typedef struct tDrvInfo {
    int cpuNum;
    int perfmgrLegacy; // /proc/perfmgr/legacy/perfserv_freq
    int perfmgrCpu;    // /proc/perfmgr/boost_ctrl/cpu_ctrl/perfserv_freq
    int ppmSupport;
    int ppmAll;        // userlimit_cpu_freq
    int acao;
    int hmp;
    int sysfs;
    int dvfs;
    int turbo;
    int hard_user_limit; // /proc/ppm/policy/hard_userlimit_cpu_freq
    int proc_powerhal_cpu_ctrl; // /proc/powerhal_cpu_ctrl
} tDrvInfo;

typedef int (*fbc_get_fstb_active)(long long);
typedef int (*fbc_wait_fstb_active)(void);
typedef int (*fbc_notify_foreground_app)(resumed_activity *, int, const char *, const char *, int, int, int, int);

/* Function prototype */
void setClusterCores(const char * scenario, int clusterNum, int totalCore, int *pCoreTbl, int *pMaxCoreTbl);
void setClusterFreq(const char * scenario, int clusterNum, int *pFreqTbl, int *pMaxFreqTbl);
void setClusterHardFreq(const char * scenario, int clusterNum, int *pFreqTbl, int *pMaxFreqTbl);
int perfScnEnable(int scenario);
int perfScnDisable(int scenario);
int perfScnUpdate(int scenario, int force_update);
void setGpuFreq(const char * scenario, int level);
void setGpuFreqMax(const char * scenario, int level);
void resetScenario(int handle, int reset_all);
void checkDrvSupport(tDrvInfo *ptDrvInfo);
int getCputopoInfo(int, int *, int *, tClusterInfo **);
int cmdSetting(int, char *, tScnNode *, int, int*);
int load_fbc_api(void);
int loadRscTable(int power_on_init);
static int check_vendor_partition(void);
int testRscTable(void);
int check_core_ctl_ioctl();
int getCPUFreq(int cid, int opp);
int init_GameMode();
void update_default_core_min();
int load_mbrain_api(void);

/* Global variable */
static int nIsReady = 0;
static int nPowerLibEnable = 1;
/*static char* sProjName = (char*)PROJ_ALL;*/
static Mutex sMutex;
static int scn_cores_now = 0;

static tClusterInfo *ptClusterTbl = NULL;
static int           nClusterNum = 0;

static tScnNode  *ptScnList = NULL;

static tDrvInfo   gtDrvInfo;
static int        nPackNum = 0;
static int        nCpuNum = 0;
static int        nUserScnBase = 0;
static int        SCN_APP_RUN_BASE = REG_SCN_MAX;
static int user_handle_now = 0;

static int nGpuFreqCount = 0;
static int nGpuHighestFreqLevel = 0;
static int scn_gpu_freq_now = 0;
static int scn_gpu_freq_max_now = 0;
static int scn_hard_gpu_freq_now = 0;
static int scn_hard_gpu_freq_max_now = 0;

//static int last_from_uid  = 1;
static int fg_launch_time_cold = 0;
static int fg_launch_time_warm = 0;
static int fg_act_switch_time = 0;
static int install_max_duration = 0;
static char foreground_pack[PACK_NAME_MAX];
static char foreground_act[PACK_NAME_MAX];
static int CpuFreqReady = 0;
static int game_mode_pid = -1;
static int notifyGbePid = 1;
static int video_mode_pid = -1;
static int camera_mode_pid = -1;
xml_activity foreground_app_info;
int (*xgfGetFstbActive)(long long) = NULL;
int (*xgfWaitFstbActive)(void) = NULL;
static int powerhalAPI_status[STATUS_API_COUNT];
int mode_status[MODE_COUNT] = {1, 0};
int isFbcSupport = 0;
static int mInteractive = 0;
static int core_ctl_dev_fd = -1;

//Mbrain
int isMbrainSupport = 0;
NotifyCpuFreqCapSetupHook notifyMbrainCpuFreqCap = nullptr;
NotifyGameModeEnabledHook notifyMBrainGameModeEnabled = nullptr;
NotifyToCloseDBHook notifyToCloseDB = nullptr;
void* libMBrainHandle = nullptr;


static nsecs_t last_touch_time = 0;
static nsecs_t last_aee_time = 0;

extern tScnConTable tConTable[FIELD_SIZE];
extern tRscConfig RscCfgTbl[];

int    gRscCtlTblLen = 0;
static tRscCtlEntry *gRscCtlTbl = NULL;

// value of softlimit, hardlimit
_cpufreq soft_freq[CLUSTER_MAX];
_cpufreq hard_freq[CLUSTER_MAX];
//SPD:porting thermal ux policy by sifengtian 20230525 start
_cpufreq thermal_freq[CLUSTER_MAX];
//SPD:porting thermal ux policy by sifengtian 20230525 end
//SPD: add powermode by rui.zhou6 20240902 start
int g_tran_power_mode_support = 0;
#define TRAN_POWER_MODE_SUPPORT "ro.vendor.power_mode.support"
#define TRAN_CPU_LIMIT_MODE "persist.vendor.powerhal.lmode"
#define TRAN_POWERHAL_ENCODE "ro.vendor.powerhal.encode"
_cpufreq tran_hard_freq[CLUSTER_MAX];
//SPD: add powermode by rui.zhou6 20240902 end
//SPD:porting thermal ux policy by sifengtian 20230525 start
extern bool thermal_ux_policy_enable;
//SPD:porting thermal ux policy by sifengtian 20230525 end

//BSP:add for thermal policy switch by jian.li at 20240424 start
char g_current_foreground_pack[PACK_NAME_MAX] = {0};
char g_current_paused_pack[PACK_NAME_MAX] = {0};
//BSP:add for thermal policy switch by jian.li at 20240424 end
// core_ctl
int default_core_min[CLUSTER_MAX] = {CORE_CTL_CPUCORE_MIN_CLUSTER_0, CORE_CTL_CPUCORE_MIN_CLUSTER_1, CORE_CTL_CPUCORE_MIN_CLUSTER_2};

int atrace_marker_fd = -1;

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
//SPD: add powerhal reinit by sifengtian 20230711 start
int reinit(int reinit_version)
{
    int i;
    const char * scn_file_path;

    /* V.10: only support thermal_ux*/
    ALOGI("[reinit] version:%d", reinit_version);
 
    if (reinit_version == 1) {
        thermalUxPolicyReInit();
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
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    const char * scn_file_path;
    const char * con_file_path;
    //SPD:porting thermal ux policy by sifengtian 20230525 end
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
        if (CpuFreqReady == 1 && getCputopoInfo(gtDrvInfo.hmp, &nCpuNum, &nClusterNum, &ptClusterTbl) != 0)
            return 0;

        /* temp for D3 */
        if (nClusterNum == 1) gtDrvInfo.hmp = 0;

        /* We won't reset CPU freq because powerhal always set it */

        //SPD: add powermode by rui.zhou6 20240902 start
        tran_powerhal_encode = get_property_value(TRAN_POWERHAL_ENCODE);
        g_tran_power_mode_support = get_property_value(TRAN_POWER_MODE_SUPPORT); 
        if(g_tran_power_mode_support) {
            init_power_mode();
            int prop_mode = get_property_value(TRAN_CPU_LIMIT_MODE);
            prop_mode = (prop_mode >= TRAN_MAX_MODE || prop_mode < 0) ? 0 : prop_mode;
            for (int i = 0; i < nClusterNum; i++) {
                tran_hard_freq[i].max = getCPUFreq(i, default_core_max[prop_mode][i]);
            }
        }
        //SPD: add powermode by rui.zhou6 20240902 end

        /* GPU info */
        get_gpu_freq_level_count(&nGpuFreqCount);
        /* Since Gpu Opp table range was from 0 to nGpuFreqCount-1, */
        /* we use nGpuFreqCount to represent free run.              */
        nGpuHighestFreqLevel = scn_gpu_freq_now = nGpuFreqCount - 1; // opp(n-1) is the lowest freq
        scn_hard_gpu_freq_now = nGpuFreqCount - 1;
        scn_gpu_freq_max_now = 0; // opp 0 is the highest freq
        scn_hard_gpu_freq_max_now = 0;
        ALOGI("nGpuFreqCount:%d", nGpuFreqCount);
        /* GPU init value */
        //setGpuFreq(0, 0);
        //setGpuFreqMax(0, nGpuHighestFreqLevel);

        for (i = 0; i < STATUS_API_COUNT; i ++)
            powerhalAPI_status[i] = 1;

        ALOGI("[init] HintRscList_init");
        HintRscList_init();
        //SPD:porting thermal ux policy by sifengtian 20230525 start
        thermalUxPolicyInit(nClusterNum);
        //SPD:porting thermal ux policy by sifengtian 20230525 end
        //SPD:porting power encode by rui.zhou6 20240902 start
        nPackNum = perfservice_xmlparse_init();

        if (nPackNum == 0) {
            ALOGE("perfservice_xmlparse_init fail");
            //return 0;
        }
        if (nPackNum >= 0) {
            if((ptScnList = (tScnNode*)malloc(sizeof(tScnNode) * (SCN_APP_RUN_BASE + nPackNum))) == NULL) {
                free(ptClusterTbl);
                ALOGE("Can't allocate memory");
                return 0;
            }
        }
        //SPD:porting power encode by rui.zhou6 20240902 end   

        // allocate memory for ptScnList
        memset(ptScnList, 0, sizeof(tScnNode)*(SCN_APP_RUN_BASE));

        /*if(gtDrvInfo.turbo && (0 == stat(CUS_CONFIG_TABLE_T, &stat_buf)))
            loadConTable(CUS_CONFIG_TABLE_T);
        else*/
        //SPD:modify config patch by fan.feng1 20241008 start
        if (access(CUS_CONFIG_TABLE_2, F_OK) != -1) {
            con_file_path = CUS_CONFIG_TABLE_2;
        } else if (access(CUS_SCN_TABLE, F_OK) != -1) {
            con_file_path = CUS_CONFIG_TABLE;
        }

        loadConTable(con_file_path);
        //SPD:modify config patch by fan.feng1 20241008 end
        update_default_core_min();

        /* Is it the first init during power on */
        property_get(POWER_PROP_INIT, prop_content, "0"); // init before ?
        prop_value = atoi(prop_content);           // prop_value:1 means powerhal init before
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

        //SPD:porting thermal ux policy by sifengtian 20230525 start
        if (access(CUS_SCN_TABLE_2, F_OK) != -1) {
            scn_file_path = CUS_SCN_TABLE_2;
        } else if (access(CUS_SCN_TABLE, F_OK) != -1) {
            scn_file_path = CUS_SCN_TABLE;
        }
 
        ALOGI("[init] updateCusScnTable:%s", scn_file_path);
 
        updateCusScnTable(scn_file_path);
        //SPD:porting thermal ux policy by sifengtian 20230525 end
        init_GameMode();
        /* Message Handler */
        createPowerMsgHandler();

        /* test mode */
        property_get(POWER_PROP_ENABLE, prop_content, "1");
        nPowerLibEnable = atoi(prop_content);

        if (load_fbc_api() < 0)
            isFbcSupport = 0;
        else
            isFbcSupport = 1;
        LOG_I("isFbcSupport %d", isFbcSupport);

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

void setClusterCores(const char * scenario, int clusterNum, int totalCore, int *pCoreTbl, int *pMaxCoreTbl)
{
    int coreToSet = 0, maxToSet = 0;
    int i;
    char str[128], buf[32];

    //if(gtDrvInfo.acao)
    //    return;

    ALOGV("[setClusterCores] scn:%s, total:%d, cores:%d, %d", scenario, totalCore, pCoreTbl[0], pCoreTbl[1]);

    if (gtDrvInfo.perfmgrCpu) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(sprintf(buf, "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_BOOST_CORE_CTRL, str);
        ALOGI("%s: cpu_ctrl set cpu core: %s", scenario, str);
    }
    else if (gtDrvInfo.perfmgrLegacy) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(sprintf(buf, "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_PERFMGR_CORE_CTRL, str);
        ALOGI("%s: legacy set cpu core: %s", scenario, str);
    }
    else if (gtDrvInfo.ppmAll) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(sprintf(buf, "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_PPM_CORE_CTRL, str);
        ALOGI("%s: ppmall set cpu core: %s", scenario, str);
    }
    else if (gtDrvInfo.ppmSupport) {
        str[0] = '\0';

        for (i=0; i<clusterNum; i++) {
            coreToSet = (pCoreTbl[i] <= 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];
            if(sprintf(buf, "%d %d ", coreToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
            set_value(PATH_PPM_CORE_BASE, i, coreToSet);
            set_value(PATH_PPM_CORE_LIMIT, i, maxToSet);
        }
        str[strlen(str)-1] = '\0'; // remove last space
        ALOGI("%s: ppmsupport set cpu core: %s", scenario, str);
    }
    else if (check_core_ctl_ioctl() == 0) {
        str[0] = '\0';
        for (i = 0; i < clusterNum; i ++) {
            coreToSet = (pCoreTbl[i] < 0 || pCoreTbl[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pCoreTbl[i];
            maxToSet = (pMaxCoreTbl[i] < 0 || pMaxCoreTbl[i] >= ptClusterTbl[i].cpuNum) ? PPM_IGNORE : pMaxCoreTbl[i];

            if(sprintf(buf, "%d %d ", coreToSet, maxToSet) < 0) {
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
        LOG_I("%s: set core_ctrl: %s", scenario, str);
    }

}

void setClusterHardFreq(const char * scenario, int clusterNum, int *pFreqTbl, int *pMaxFreqTbl)
{
    int freqToSet = 0, maxToSet = 0;
    int i;
    char str[128], buf[32];

    ALOGV("[setClusterHardFreq] scn:%s, freq:%d, %d , %d, %d", scenario, pFreqTbl[0], pMaxFreqTbl[0],pFreqTbl[1], pMaxFreqTbl[1]);
    if (gtDrvInfo.hard_user_limit) {
        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            maxToSet = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];
            if(sprintf(buf, "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_HARD_USER_LIMIT, str);
        ALOGI("%s: hard user limit set cpu freq: %s", scenario, str);
    }
    else {
#if HARDLIMIT_ENABLED
        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            maxToSet = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];
            if(sprintf(buf, "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0';
        LOG_I("%s: hard userlimit set cpu freq: %s", scenario, str);
        setClusterFreq(scenario, clusterNum, pFreqTbl, pMaxFreqTbl);
#else
        LOG_E("Hard userlimit Not Supported.");
#endif
    }
}
void setClusterFreq(const char * scenario, int clusterNum, int *pFreqTbl, int *pMaxFreqTbl)
{
    int freqToSet = 0, maxToSet = 0;
    int i;
    char str[128], buf[32];
    char final_str[128], final_buf[32];

    ALOGV("[setClusterFreq] scn:%s, freq:%d, %d", scenario, pFreqTbl[0], pFreqTbl[1]);
    if (gtDrvInfo.perfmgrCpu) {
        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            //freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : (pFreqTbl[i] > ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pFreqTbl[i];
            freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            maxToSet = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];
            if(sprintf(buf, "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_BOOST_FREQ_CTRL, str);
        ALOGI("%s: cpu_ctrl set cpu freq: %s", scenario, str);
    }
    else if (gtDrvInfo.perfmgrLegacy) {
        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            //freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : (pFreqTbl[i] > ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pFreqTbl[i];
            freqToSet = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            maxToSet = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];
            if(sprintf(buf, "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            /*strcat(str, buf);*/
            strncat(str, buf, strlen(buf));
        }
        str[strlen(str)-1] = '\0'; // remove last space
        set_value(PATH_PERFMGR_FREQ_CTRL, str);
        ALOGI("%s: legacy set cpu freq: %s", scenario, str);
    } else if (gtDrvInfo.proc_powerhal_cpu_ctrl) {
         //add for balance mode by rui.zhou6 at 20240902 start
     	if (g_tran_power_mode_support) {
            int prop_mode = get_property_value(TRAN_CPU_LIMIT_MODE);
            prop_mode = (prop_mode >= TRAN_MAX_MODE || prop_mode < 0) ? 0 : prop_mode;
            for (int i = 0; i < nClusterNum; i++) {
                 tran_hard_freq[i].max = getCPUFreq(i, default_core_max[prop_mode][i]);
            } 
     	}
        //add for balance mode by rui.zhou6 at 20240902 end
        for (i = 0; i < clusterNum; i ++) {
            LOG_D("cluster_%d soft:%d %d, hard:%d %d", i,
                soft_freq[i].min, (soft_freq[i].max == FREQ_MAX) ? ptClusterTbl[i].freqMax : soft_freq[i].max,
                hard_freq[i].min, (hard_freq[i].max == FREQ_MAX) ? ptClusterTbl[i].freqMax : hard_freq[i].max);
        }

        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            int min_freq = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            int max_freq = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];

            pFreqTbl[i] = soft_freq[i].min;
            pMaxFreqTbl[i] = soft_freq[i].max;
            freqToSet = (pFreqTbl[i] <= ptClusterTbl[i].freqMin) ? ptClusterTbl[i].freqMin : ((pFreqTbl[i] >= ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pFreqTbl[i]);
            if (pMaxFreqTbl[i] <= 0)
                maxToSet = ptClusterTbl[i].freqMax;
            else
                maxToSet = (pMaxFreqTbl[i] <= ptClusterTbl[i].freqMin) ? ptClusterTbl[i].freqMin : ((pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pMaxFreqTbl[i]);
#if HARDLIMIT_ENABLED
            // compare with hardlimit
            if (soft_freq[i].min < hard_freq[i].min) {
                freqToSet = hard_freq[i].min;
            }
            if (soft_freq[i].min > hard_freq[i].max) {
                freqToSet = hard_freq[i].max;
            }
            if (soft_freq[i].max > hard_freq[i].max) {
                maxToSet = hard_freq[i].max;
            }
            if (soft_freq[i].max < hard_freq[i].min) {
                maxToSet = hard_freq[i].min;
            }
            //SPD:porting thermal ux policy by sifengtian 20230525 start
            if (thermal_ux_policy_enable) {
                if (thermal_freq[i].max != THERMAL_UX_CPU_MAX_RESET && is_valid_temp()) {
                    thermal_freq[i].max = thermal_freq[i].max > ptClusterTbl[i].freqMax ? ptClusterTbl[i].freqMax : thermal_freq[i].max;
                    maxToSet = maxToSet < thermal_freq[i].max ? thermal_freq[i].max : maxToSet;
                    freqToSet = (pFreqTbl[i] <= ptClusterTbl[i].freqMin) ? ptClusterTbl[i].freqMin : ((pFreqTbl[i] >= maxToSet) ? maxToSet : pFreqTbl[i]);
                    ALOGI("[thermal ux] update cluster%d to freqToSet:%d maxToSet:%d", i, freqToSet, maxToSet);
                }
            }
            //SPD:porting thermal ux policy by sifengtian 20230525 end
            //add for balance mode by rui.zhou6 at 20240902 start
            if (g_tran_power_mode_support) {
                    if (maxToSet > tran_hard_freq[i].max && hard_freq[i].max == CPU_FREQ_MAX_RESET) {
                        maxToSet = tran_hard_freq[i].max;
                    }
                    if (freqToSet > tran_hard_freq[i].max && hard_freq[i].min == CPU_FREQ_MIN_RESET) {
                        freqToSet = tran_hard_freq[i].max;
                    }
            }
            //add for balance mode by rui.zhou6 at 20240902 end
            if(sprintf(buf, "%d %d ", freqToSet, maxToSet) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));

            int final_min_freq = (freqToSet <= ptClusterTbl[i].freqMin) ? PPM_IGNORE : freqToSet;
            int final_max_freq = (maxToSet <= 0 || ptClusterTbl[i].freqMax <= maxToSet) ? PPM_IGNORE : maxToSet;
            if (sprintf(final_buf, "%d %d ", final_min_freq, final_max_freq) < 0) {
                LOG_E("sprintf error");
                return;
            }
            strncat(final_str, final_buf, strlen(final_buf));
#endif
        }
        str[strlen(str)-1] = '\0'; // remove last space
        if (set_value(PATH_PROC_CPU_CTRL, str) == 0) {
            notifyWriteFail();
        }

#if HARDLIMIT_ENABLED
        LOG_I(" set cpu_ctrl cpufreq: %s", final_str);
#else
        ALOGI(" set cpu_ctrl cpufreq: %s", str);
#endif

    } else {
        for (i = 0; i < clusterNum; i ++) {
            LOG_D("cluster_%d soft:%d %d, hard:%d %d", i,
                soft_freq[i].min, (soft_freq[i].max == FREQ_MAX) ? ptClusterTbl[i].freqMax : soft_freq[i].max,
                hard_freq[i].min, (hard_freq[i].max == FREQ_MAX) ? ptClusterTbl[i].freqMax : hard_freq[i].max);
        }

        str[0] = '\0';
        for (i=0; i<clusterNum; i++) {
            int min_freq = (pFreqTbl[i] <= 0) ? PPM_IGNORE : pFreqTbl[i];
            int max_freq = (pMaxFreqTbl[i] <= 0 || pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? PPM_IGNORE : pMaxFreqTbl[i];
            if(sprintf(buf, "%d %d ", min_freq, max_freq) < 0) {
                ALOGE("sprintf error");
                return;
            }
            strncat(str, buf, strlen(buf));

            pFreqTbl[i] = soft_freq[i].min;
            pMaxFreqTbl[i] = soft_freq[i].max;
            freqToSet = (pFreqTbl[i] <= ptClusterTbl[i].freqMin) ? ptClusterTbl[i].freqMin : ((pFreqTbl[i] >= ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pFreqTbl[i]);
            if (pMaxFreqTbl[i] <= 0)
                maxToSet = ptClusterTbl[i].freqMax;
            else
                maxToSet = (pMaxFreqTbl[i] <= ptClusterTbl[i].freqMin) ? ptClusterTbl[i].freqMin : ((pMaxFreqTbl[i] >= ptClusterTbl[i].freqMax) ? ptClusterTbl[i].freqMax : pMaxFreqTbl[i]);
#if HARDLIMIT_ENABLED
            // compare with hardlimit
            if (soft_freq[i].min < hard_freq[i].min) {
                freqToSet = hard_freq[i].min;
            }
            if (soft_freq[i].min > hard_freq[i].max) {
                freqToSet = hard_freq[i].max;
            }
            if (soft_freq[i].max > hard_freq[i].max) {
                maxToSet = hard_freq[i].max;
            }
            if (soft_freq[i].max < hard_freq[i].min) {
                maxToSet = hard_freq[i].min;
            }
            int final_min_freq = (freqToSet <= ptClusterTbl[i].freqMin) ? PPM_IGNORE : freqToSet;
            int final_max_freq = (maxToSet <= 0 || ptClusterTbl[i].freqMax <= maxToSet) ? PPM_IGNORE : maxToSet;
            if (sprintf(final_buf, "%d %d ", final_min_freq, final_max_freq) < 0) {
                LOG_E("sprintf error");
                return;
            }
            strncat(final_str, final_buf, strlen(final_buf));
#endif
            if ((set_value(ptClusterTbl[i].sysfsMinPath, freqToSet) == 0) || (set_value(ptClusterTbl[i].sysfsMaxPath, maxToSet) == 0)) {
                notifyWriteFail();
            }
        }
        str[strlen(str)-1] = '\0'; // remove last space
#if HARDLIMIT_ENABLED
        LOG_I("%s: sysfs_freq set cpu freq: %s", scenario, final_str);
#else
        ALOGI("%s: sysfs_freq set cpu freq: %s", scenario, str);
#endif
    }
}

void setGpuFreq(const char * scenario, int level)
{
    int levelToSet = 0;
    static int nSetFreqInit = 0;
    static int nIsGpuFreqSupport = 0;
    struct stat stat_buf;

    if(!nSetFreqInit) {
        nIsGpuFreqSupport = (0 == stat(PATH_GPUFREQ_COUNT, &stat_buf)) ? 1 : 0;
        nSetFreqInit = 1;
    }

    if(!nIsGpuFreqSupport)
        return;

    /*-- nGpuFreqCount means free run      --*/
    /*-- lowest index opp give to free run --*/
    if(level >= nGpuFreqCount)
        levelToSet = (nGpuFreqCount - 1);
    else
        levelToSet = level;
    ALOGI("%s: set gpu opp level: %d", scenario, levelToSet);
    set_gpu_freq_level(levelToSet); // 0 means maximum
}

void setGpuFreqMax(const char * scenario, int level)
{
    /*-- Since we use 0 to compare with other setting,--*/
    /*-- we don't need to use the condition check.   --*/
    /*-- nGpuFreqCount means free run       --*/
    /*-- highest index opp give to free run --*/
    /*-- if(level == nGpuFreqCount) levelToSet = 0;   --*/
    ALOGI("%s: set gpu opp level max: %d", scenario, level);
    set_gpu_freq_level_max(level); // 0 means maximun
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

int getCputopoInfo(int isHmpSupport, int *pnCpuNum, int *pnClusterNum, tClusterInfo **pptClusterTbl)
{
    int i, j;
    int cpu_num[CLUSTER_MAX], cpu_index[CLUSTER_MAX];
    int cputopoClusterNum = 1;
    struct stat stat_buf;

    for (i=0; i<CLUSTER_MAX; i++)
        cpu_num[i] = cpu_index[i] = 0;

    *pnCpuNum = get_cpu_num();

    if(0 == stat(PATH_PERFMGR_TOPO_NR_CLUSTER, &stat_buf))
        cputopoClusterNum = get_int_value(PATH_PERFMGR_TOPO_NR_CLUSTER);
    else if(0 == stat(PATH_CPUTOPO_NR_CLUSTER, &stat_buf))
        cputopoClusterNum = get_int_value(PATH_CPUTOPO_NR_CLUSTER);
    else
        getCputopoFromSysfs(*pnCpuNum, &cputopoClusterNum, NULL, NULL);

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
        getCputopoFromSysfs(*pnCpuNum, &cputopoClusterNum, cpu_num, cpu_index);
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
        //SPD:porting thermal ux policy by sifengtian 20230525 start
        (*pptClusterTbl)[i].freqThermalUxNow = 0;
        //SPD:porting thermal ux policy by sifengtian 20230525 end
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

static void notifyMBrainCloseDB()
{
    if( (0 != isMbrainSupport) && (nullptr != notifyToCloseDB) ) {
        notifyToCloseDB();
    }
}

void processMBrain(){
    notifyMBrainCloseDB();
}

int perfScnEnable(int scenario)
{
    int needUpdateCores = 0, needUpdateCoresMax = 0, needUpdateFreq = 0, needUpdateFreqMax = 0, i = 0;
    int scn_core_min[CLUSTER_MAX], actual_core_min[CLUSTER_MAX], totalCore, coreToSet;
    int scn_core_max[CLUSTER_MAX];
    int scn_freq_min[CLUSTER_MAX];
    int scn_freq_max[CLUSTER_MAX];
    int scn_freq_hard_min[CLUSTER_MAX];
    int scn_freq_hard_max[CLUSTER_MAX];
    int needUpdateHardFreq = 0;
    int result;
    char scn_name[PACK_NAME_MAX];
    int log_hasI = 1;
    char str[128], buf[128], final_str[128], final_buf[128];
    int cpufreq_set = 0;
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    int needUpdateThermalUxFreq = 0;
    int scn_freq_thermal_ux_max[CLUSTER_MAX];
    //SPD:porting thermal ux policy by sifengtian 20230525 end

    if (checkSuccess(scenario)) {
        //if (STATE_ON == ptScnList[scenario].scn_state)
        //    return 0;

        GetScnName(scenario, "PE", scn_name);
        ptScnList[scenario].scn_state = STATE_ON;

        ALOGV("[perfScnEnable] scn:%s, scn_cores_now:%d, scn_core_total:%d",
            scn_name, scn_cores_now, ptScnList[scenario].scn_core_total);

        if (scn_cores_now < ptScnList[scenario].scn_core_total) {
            scn_cores_now = ptScnList[scenario].scn_core_total;
            needUpdateCores = 1;
        }

        for (i=0; i<nClusterNum; i++) {

            if (ptScnList[scenario].scn_core_min[i] != CPU_CORE_MIN_RESET || ptScnList[scenario].scn_core_max[i] != CPU_CORE_MAX_RESET)
                ALOGI("[setClusterCores] scn:%s set cpu core[%d] min:%d max:%d",
                    scn_name, i, ptScnList[scenario].scn_core_min[i],
                    ptScnList[scenario].scn_core_max[i] != CPU_CORE_MAX_RESET ? ptScnList[scenario].scn_core_max[i] : -1);

            if (ptClusterTbl[i].cpuMinNow < ptScnList[scenario].scn_core_min[i]) {
                ptClusterTbl[i].cpuMinNow = ptScnList[scenario].scn_core_min[i];
                needUpdateCores = 1;
            }
            scn_core_min[i] = ptClusterTbl[i].cpuMinNow;

            //ALOGV("[perfScnEnable] scn:%d, i:%d, cpuMaxNow:%d, scn_core_max:%d", scenario, i, ptClusterTbl[i].cpuMaxNow, ptScnList[scenario].scn_core_max[i]);
            if ((ptClusterTbl[i].cpuMaxNow > ptScnList[scenario].scn_core_max[i] || ptClusterTbl[i].cpuMaxNow == PPM_IGNORE)) {
                ptClusterTbl[i].cpuMaxNow = ptScnList[scenario].scn_core_max[i];
                needUpdateCoresMax = 1;
            }

            if (ptScnList[scenario].scn_freq_min[i] != CPU_FREQ_MIN_RESET || ptScnList[scenario].scn_freq_max[i] != CPU_FREQ_MAX_RESET) {
                cpufreq_set = 1;
                ALOGD("[setClusterFreq] scn:%s set cpu freq[%d] min:%d max:%d",
                    scn_name, i, ptScnList[scenario].scn_freq_min[i],
                    ptScnList[scenario].scn_freq_max[i] != CPU_FREQ_MAX_RESET ? ptScnList[scenario].scn_freq_max[i] : -1);
            }

            if (sprintf(buf, "cpufreq[%d] min:%d max:%d ", i, ptScnList[scenario].scn_freq_min[i], (ptScnList[scenario].scn_freq_max[i] != CPU_FREQ_MAX_RESET) ? ptScnList[scenario].scn_freq_max[i] : -1) < 0) {
                LOG_E("sprintf error");
                return 0;
            }
            strncat(str, buf, strlen(buf));

            ALOGV("[perfScnEnable] scn:%s, i:%d, freqMinNow:%d, scn_freq_min:%d",
                scn_name, i, ptClusterTbl[i].freqMinNow, ptScnList[scenario].scn_freq_min[i]);
            if (ptClusterTbl[i].freqMinNow < ptScnList[scenario].scn_freq_min[i]) {
                ptClusterTbl[i].freqMinNow = ptScnList[scenario].scn_freq_min[i];
                needUpdateFreq = 1;
                notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 1, ptScnList[scenario].scn_freq_min[i]);
            }
            scn_freq_min[i] = ptClusterTbl[i].freqMinNow;

            ALOGV("[perfScnEnable] scn:%s, i:%d, freqMaxNow:%d, scn_freq_max:%d",
                scn_name, i, ptClusterTbl[i].freqMaxNow, ptScnList[scenario].scn_freq_max[i]);
            if (ptClusterTbl[i].freqMaxNow > ptScnList[scenario].scn_freq_max[i]) {
                ptClusterTbl[i].freqMaxNow = ptScnList[scenario].scn_freq_max[i];
                needUpdateFreqMax = 1;
                notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 2, ptScnList[scenario].scn_freq_max[i]);
            }
            if (ptClusterTbl[i].freqMaxNow < scn_freq_min[i]) { // if max < min => align max with min
                ptClusterTbl[i].freqMaxNow = scn_freq_min[i];
                needUpdateFreqMax = 1;
            }
            scn_freq_max[i] = ptClusterTbl[i].freqMaxNow;

            if (ptScnList[scenario].scn_freq_hard_min[i] != CPU_FREQ_MIN_RESET || ptScnList[scenario].scn_freq_hard_max[i] != CPU_FREQ_MAX_RESET)
                ALOGI("[setClusterFreq] scn:%s set cpu hard_freq[%d] min:%d max:%d",
                    scn_name, i, ptScnList[scenario].scn_freq_hard_min[i],
                    ptScnList[scenario].scn_freq_hard_max[i] != CPU_FREQ_MAX_RESET ? ptScnList[scenario].scn_freq_hard_max[i] : -1);

            ALOGV("[perfScnEnable] scn:%s, i:%d, freqHardMinNow:%d, scn_freq_hard_min:%d", scn_name, i, ptClusterTbl[i].freqHardMinNow, ptScnList[scenario].scn_freq_hard_min[i]);
            if (ptClusterTbl[i].freqHardMinNow < ptScnList[scenario].scn_freq_hard_min[i]) {
                ptClusterTbl[i].freqHardMinNow = ptScnList[scenario].scn_freq_hard_min[i];
                needUpdateHardFreq = 1;
            }
            scn_freq_hard_min[i] = ptClusterTbl[i].freqHardMinNow;

            ALOGV("[perfScnEnable] scn:%s, i:%d, freqHardMaxNow:%d, scn_freq_hard_max:%d", scn_name, i, ptClusterTbl[i].freqHardMaxNow, ptScnList[scenario].scn_freq_hard_max[i]);
            if (ptClusterTbl[i].freqHardMaxNow > ptScnList[scenario].scn_freq_hard_max[i]) {
                ptClusterTbl[i].freqHardMaxNow = ptScnList[scenario].scn_freq_hard_max[i];
                needUpdateHardFreq = 1;
            }
            if (ptClusterTbl[i].freqHardMaxNow < scn_freq_hard_min[i]) { // if max < min => align ceiling with floor
                ptClusterTbl[i].freqHardMaxNow = scn_freq_hard_min[i];
                needUpdateHardFreq = 1;
            }
            scn_freq_hard_max[i] = ptClusterTbl[i].freqHardMaxNow;
            //SPD:porting thermal ux policy by sifengtian 20230525 start
            ALOGD("[perfScnEnable] scn:%s, i:%d, last_thermal_ux_maxfreq:%d", scn_name, i, ptClusterTbl[i].freqThermalUxNow);
            if (ptClusterTbl[i].freqThermalUxNow < ptScnList[scenario].scn_freq_thermal_ux_max[i]) {
                ptClusterTbl[i].freqThermalUxNow = ptScnList[scenario].scn_freq_thermal_ux_max[i];
                needUpdateThermalUxFreq = 1;
            }
            scn_freq_thermal_ux_max[i] = ptClusterTbl[i].freqThermalUxNow;
            ALOGD("[perfScnEnable] scn:%s, i:%d, cur_thermal_ux_maxfreq:%d", scn_name, i, ptClusterTbl[i].freqThermalUxNow);
            //SPD:porting thermal ux policy by sifengtian 20230525 end
        }
        if (cpufreq_set == 1)
            ALOGI("[setClusterFreq] %s set %s", scn_name, str);

        /*--gpu opp used start--*/
        if ((ptScnList[scenario].scn_gpu_freq != -1) || (ptScnList[scenario].scn_gpu_freq_max != -1) || (ptScnList[scenario].scn_gpu_freq_hard_min != -1) || (ptScnList[scenario].scn_gpu_freq_hard_max != -1)) {
            /*-- If scn_gpu-freq was equal to -1 which mean don't care, --*/
            /*-- we don't do anything and keep the scn_gpu_freq_now     --*/
            ALOGI("[setGPUFreq] %s user set gpu freq min:%d, max:%d", scn_name, ptScnList[scenario].scn_gpu_freq, ptScnList[scenario].scn_gpu_freq_max);
            if ((ptScnList[scenario].scn_gpu_freq_hard_min != -1) || (ptScnList[scenario].scn_gpu_freq_hard_max != -1))
                ALOGI("[setGPUFreq] %s user set Hard gpu freq min:%d, max:%d", scn_name, ptScnList[scenario].scn_gpu_freq_hard_min, ptScnList[scenario].scn_gpu_freq_hard_max);

            if ((ptScnList[scenario].scn_gpu_freq != -1) &&
                (scn_gpu_freq_now > ptScnList[scenario].scn_gpu_freq)) {
                 scn_gpu_freq_now = ptScnList[scenario].scn_gpu_freq;
                 setGpuFreq(scn_name, scn_gpu_freq_now);
            }

            /*-- check freq max --*/
            if ((ptScnList[scenario].scn_gpu_freq_max != -1) &&
                (scn_gpu_freq_max_now < ptScnList[scenario].scn_gpu_freq_max))
                scn_gpu_freq_max_now = ptScnList[scenario].scn_gpu_freq_max;

            if (scn_gpu_freq_max_now > scn_gpu_freq_now)
                scn_gpu_freq_max_now = scn_gpu_freq_now;

            setGpuFreqMax(scn_name, scn_gpu_freq_max_now);

#if GPU_HARDLIMIT_ENABLED
            int freqToSet = scn_gpu_freq_now;
            int maxToSet = scn_gpu_freq_max_now;
            int final_min_freq = -1, final_max_freq = -1;

            if ((ptScnList[scenario].scn_gpu_freq_hard_min != -1) && (scn_hard_gpu_freq_now > ptScnList[scenario].scn_gpu_freq_hard_min)) {
                scn_hard_gpu_freq_now = ptScnList[scenario].scn_gpu_freq_hard_min;
            }
            if ((ptScnList[scenario].scn_gpu_freq_hard_max != -1) && (scn_hard_gpu_freq_max_now < ptScnList[scenario].scn_gpu_freq_hard_max))
                scn_hard_gpu_freq_max_now = ptScnList[scenario].scn_gpu_freq_hard_max;
            if (scn_hard_gpu_freq_max_now > scn_hard_gpu_freq_now)
                scn_hard_gpu_freq_max_now = scn_hard_gpu_freq_now;

            ALOGI("[setGPUFreq] Soft min/max = (%d, %d); Hard min/max = (%d, %d)", scn_gpu_freq_now, scn_gpu_freq_max_now, scn_hard_gpu_freq_now, scn_hard_gpu_freq_max_now);
            if (scn_gpu_freq_now > scn_hard_gpu_freq_now) {
                freqToSet = scn_hard_gpu_freq_now;
            }
            if (scn_gpu_freq_now < scn_hard_gpu_freq_max_now) {
                freqToSet = scn_hard_gpu_freq_max_now;
            }
            if (scn_gpu_freq_max_now < scn_hard_gpu_freq_max_now) {
                maxToSet = scn_hard_gpu_freq_max_now;
            }
            if (scn_gpu_freq_max_now > scn_hard_gpu_freq_now) {
                maxToSet = scn_hard_gpu_freq_now;
            }
            final_min_freq = freqToSet;
            final_max_freq = maxToSet;
            ALOGI("[setGPUFreq] final min/max = (%d, %d)", final_min_freq, final_max_freq);
            setGpuFreq(scn_name, final_min_freq);
            setGpuFreqMax(scn_name, final_max_freq);
#endif
        } /*--gpu opp used end--*/

        // fine tune max
        totalCore = scn_cores_now;
        for (i=nClusterNum-1; i>=0; i--) {
            coreToSet = (scn_core_min[i] < 0 || scn_core_min[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : scn_core_min[i];
            if(coreToSet >= 0)
                totalCore -= coreToSet;
            actual_core_min[i] = coreToSet;

            if (ptClusterTbl[i].cpuMaxNow < actual_core_min[i]) { // min priority is higher than max
                ptClusterTbl[i].cpuMaxNow = actual_core_min[i];
                needUpdateCoresMax = 1;
            }
            scn_core_max[i] = ptClusterTbl[i].cpuMaxNow;
        }

        // L and LL: only one cluster can set max cpu = 0
        if (nClusterNum > 1 && ptClusterTbl[1].cpuMaxNow == 0 && ptClusterTbl[0].cpuMaxNow == 0) {
            ptClusterTbl[0].cpuMaxNow = scn_core_max[0] = PPM_IGNORE;
            needUpdateCoresMax = 1;
        }

        //SPD:porting thermal ux policy by sifengtian 20230525 start
        if (needUpdateThermalUxFreq && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                thermal_freq[cid].max = scn_freq_thermal_ux_max[cid];
            }
            if(!needUpdateHardFreq) {
                needUpdateHardFreq = 1;
            }
        }
        //SPD:porting thermal ux policy by sifengtian 20230525 end

        if ((needUpdateFreq || needUpdateFreqMax) && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                soft_freq[cid].min = scn_freq_min[cid];
                soft_freq[cid].max = scn_freq_max[cid];
                hard_freq[cid].min = scn_freq_hard_min[cid];
                hard_freq[cid].max = scn_freq_hard_max[cid];
            }
            setClusterFreq(scn_name, nClusterNum, scn_freq_min, scn_freq_max);
        }

        if (needUpdateHardFreq && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                soft_freq[cid].min = scn_freq_min[cid];
                soft_freq[cid].max = scn_freq_max[cid];
                hard_freq[cid].min = scn_freq_hard_min[cid];
                hard_freq[cid].max = scn_freq_hard_max[cid];
            }
            setClusterHardFreq(scn_name, nClusterNum, scn_freq_hard_min, scn_freq_hard_max);
        }

        if (needUpdateCoresMax || needUpdateCores) {
            setClusterCores(scn_name, nClusterNum, scn_cores_now, actual_core_min, scn_core_max);
        }

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
                ALOGD("[perfScnEnable] scn:%s, user set cmdName:%s value:%d cur:%d isValid:%d",
                   scn_name, tConTable[idx].cmdName.c_str(), ptScnList[scenario].scn_param[idx],
                   tConTable[idx].curVal, tConTable[idx].isValid);

            if(tConTable[idx].isValid == -1)
                continue;

            if(tConTable[idx].comp.compare(LESS) == 0) {
                if(ptScnList[scenario].scn_param[idx] < tConTable[idx].curVal)
                {
                    tConTable[idx].curVal = ptScnList[scenario].scn_param[idx];
                    if(tConTable[idx].prefix.length() == 0) {
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), ptScnList[scenario].scn_param[idx])) {
                                if(log_hasI)
                                    ALOGI("[PE] %s update cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx]);
                                else
                                    ALOGD("[PE] %s update cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx]);
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
                if(ptScnList[scenario].scn_param[idx] > tConTable[idx].curVal)
                {
                    tConTable[idx].curVal = ptScnList[scenario].scn_param[idx];
                    if(tConTable[idx].prefix.length() == 0) {
                        if(tConTable[idx].entry.length() > 0) {
                            if (set_value(tConTable[idx].entry.c_str(), ptScnList[scenario].scn_param[idx])) {
                                if(log_hasI)
                                    ALOGI("[PE] %s update cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx]);
                                else
                                    ALOGD("[PE] %s update cmd:%x, param:%d", scn_name, tConTable[idx].cmdID, ptScnList[scenario].scn_param[idx]);
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

            if(RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL)
                break;

            if (gRscCtlTbl[idx].resetVal != ptScnList[scenario].scn_rsc[idx])
                ALOGD("[perfScnEnable] scn:%s user set: cmdName:%s param:%d cur:%d isValid:%d",
                    scn_name, RscCfgTbl[idx].cmdName.c_str(), ptScnList[scenario].scn_rsc[idx],
                    gRscCtlTbl[idx].curVal, gRscCtlTbl[idx].isValid);

            if(gRscCtlTbl[idx].isValid != 1)
                continue;

            if(RscCfgTbl[idx].comp == SMALLEST) {
                if(ptScnList[scenario].scn_rsc[idx] < gRscCtlTbl[idx].curVal)
                {
                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    } else {
                        ALOGD("[PE] %s update cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    }
                }
            } else if(RscCfgTbl[idx].comp == BIGGEST) {
                if(ptScnList[scenario].scn_rsc[idx] > gRscCtlTbl[idx].curVal)
                {
                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    } else {
                        ALOGD("[PE] %s update cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    }
                }
            } else {
                if(ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal)
                {
                    if (mode_status[GAME_MODE] == 1) {
                        if (RscCfgTbl[idx].gameVal != RSC_TBL_INVALID_VALUE) {
                            result = RscCfgTbl[idx].unset_func(RscCfgTbl[idx].gameVal, (void*)&ptScnList[scenario]);
                            ALOGI("[PD] %s update ONESHOT(unset) cmd:%x param:%d", scn_name, RscCfgTbl[idx].cmdID, RscCfgTbl[idx].gameVal);
                        }
                    }

                    gRscCtlTbl[idx].curVal = ptScnList[scenario].scn_rsc[idx];
                    result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    if(log_hasI) {
                        ALOGI("[PE] %s update ONESHOT(set) cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    } else {
                        ALOGD("[PE] %s update ONESHOT(set) cmd:%x param:%d",
                            scn_name, RscCfgTbl[idx].cmdID, gRscCtlTbl[idx].curVal);
                    }
                }
            }
        }
    }

    return 0;
}

int perfScnDisable(int scenario)
{
    return perfScnUpdate(scenario, 0);
}

/*
    force_update == 0 => disable scenario
    force_update == 1 => scan all scenario and update setting
 */
int perfScnUpdate(int scenario, int force_update)
{
    int needUpdateCores = 0;
    int totalCoresToSet = 0;
    /*-- nGpuFreqCount means free run for base --*/
    int gpuFreqToSet = nGpuHighestFreqLevel; // opp(n-1) is the lowest freq
    int gpuFreqToSet_HardLimit = nGpuHighestFreqLevel;
    /*-- 0 means free run for upbound --*/
    int gpuFreqMaxToSet = 0; // opp 0
    int gpuFreqMaxToSet_HardLimit = 0;
    int needUpdate = 0;
    int coresToSet[CLUSTER_MAX], actual_core_min[CLUSTER_MAX], maxCoresToSet[CLUSTER_MAX], lastCore[CLUSTER_MAX];
    int freqToSet[CLUSTER_MAX], lastFreq[CLUSTER_MAX], maxFreqToSet[CLUSTER_MAX], lastGpuFreq, lastGpuMaxFreq;
    int lastGpuFreq_HardLimit, lastGpuMaxFreq_HardLimit;
    int totalCore, coreToSet, numToSet, numofCurr;
    int i, j;
    int hardFreqToSet[CLUSTER_MAX], lastHardFreq[CLUSTER_MAX], maxHardFreqToSet[CLUSTER_MAX];
    int needUpdateHardFreq = 0;
    int result = 0;
    char scn_name[PACK_NAME_MAX];
    char func_name[PACK_NAME_MAX];
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    int maxThermalUxFreqToSet[CLUSTER_MAX];
    int needUpdateThermalUxFreq = 0;
    //SPD:porting thermal ux policy by sifengtian 20230525 end
    if (checkSuccess(scenario)) {
        if (STATE_OFF == ptScnList[scenario].scn_state)
            return 0;

        set_str_cpy(scn_name, ptScnList[scenario].comm, PACK_NAME_MAX);

        if (force_update) {
            set_str_cpy(func_name, "PU", PACK_NAME_MAX);
            ALOGI("[%s] scn:%d, update", func_name, scenario);
        }
        else {
            set_str_cpy(func_name, "PD", PACK_NAME_MAX);
            GetScnName(scenario, func_name, scn_name);
            ptScnList[scenario].scn_state = STATE_OFF;
        }

        // check cpufreq 
        needUpdate = needUpdateHardFreq = 0;
        for (i=0; i<nClusterNum; i++) {
            /* CPU freq floor */
            lastFreq[i] = ptClusterTbl[i].freqMinNow;
            if (ptClusterTbl[i].freqMinNow <= ptScnList[scenario].scn_freq_min[i] || force_update) {
                freqToSet[i] = 0;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        freqToSet[i] = max(freqToSet[i], ptScnList[j].scn_freq_min[i]);
                }
                if(freqToSet[i] != ptClusterTbl[i].freqMinNow) {
                    ptClusterTbl[i].freqMinNow = freqToSet[i];
                    needUpdate = 1;
                }
            }
            else {
                freqToSet[i] = ptClusterTbl[i].freqMinNow;
            }

            lastHardFreq[i] = ptClusterTbl[i].freqHardMinNow;
            if (ptClusterTbl[i].freqHardMinNow <= ptScnList[scenario].scn_freq_hard_min[i] || force_update) {
                hardFreqToSet[i] = 0;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        hardFreqToSet[i] = max(hardFreqToSet[i], ptScnList[j].scn_freq_hard_min[i]);
                }
                if(hardFreqToSet[i] != ptClusterTbl[i].freqHardMinNow) {
                    ptClusterTbl[i].freqHardMinNow = hardFreqToSet[i];
                    needUpdateHardFreq = 1;
                }
            }
            else {
                hardFreqToSet[i] = ptClusterTbl[i].freqHardMinNow;
            }

            /* CPU freq ceiling */
            ALOGV("[%s] scn:%s, i:%d, last_min:%d, global_max:%d, max:%d",
                func_name, scn_name, i, lastFreq[i], ptClusterTbl[i].freqMaxNow, ptScnList[scenario].scn_freq_max[i]);
            if (ptClusterTbl[i].freqMaxNow >= ptScnList[scenario].scn_freq_max[i] || \
                ptClusterTbl[i].freqMaxNow == lastFreq[i] || force_update) { // perfservice might ignore someone's setting before
                maxFreqToSet[i] = FREQ_MAX;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        maxFreqToSet[i] = min(maxFreqToSet[i], ptScnList[j].scn_freq_max[i]);
                }
                if(maxFreqToSet[i] < freqToSet[i]) { // if max < min => align max with min
                    maxFreqToSet[i] = freqToSet[i];
                }
                if(maxFreqToSet[i] != ptClusterTbl[i].freqMaxNow) {
                    ptClusterTbl[i].freqMaxNow = maxFreqToSet[i];
                    needUpdate = 1;
                }
            }
            else {
                maxFreqToSet[i] = ptClusterTbl[i].freqMaxNow;
            }

            ALOGV("[%s] scn:%s, i:%d, last_hard_min:%d, global_hard_max:%d, max:%d",
                func_name, scn_name, i, lastHardFreq[i], ptClusterTbl[i].freqHardMaxNow, ptScnList[scenario].scn_freq_max[i]);
            if (ptClusterTbl[i].freqHardMaxNow >= ptScnList[scenario].scn_freq_hard_max[i] || \
                ptClusterTbl[i].freqHardMaxNow == lastHardFreq[i] || force_update) { // perfservice might ignore someone's setting before
                maxHardFreqToSet[i] = FREQ_MAX;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        maxHardFreqToSet[i] = min(maxHardFreqToSet[i], ptScnList[j].scn_freq_hard_max[i]);
                }
                if(maxHardFreqToSet[i] < hardFreqToSet[i]) { // if max < min => align max with min
                    maxHardFreqToSet[i] = hardFreqToSet[i];
                }
                if(maxHardFreqToSet[i] != ptClusterTbl[i].freqHardMaxNow) {
                    ptClusterTbl[i].freqHardMaxNow = maxHardFreqToSet[i];
                    needUpdateHardFreq = 1;
                }
            }
            else {
                maxHardFreqToSet[i] = ptClusterTbl[i].freqHardMaxNow;
            }
            //SPD:porting thermal ux policy by sifengtian 20230525 start
            /* thermal ux Freq ceiling*/
            ALOGD("[%s] scn:%s, i:%d, last_thermal_ux_maxfreq:%d", func_name, scn_name, i, ptClusterTbl[i].freqThermalUxNow);
            if (ptClusterTbl[i].freqThermalUxNow <= ptScnList[scenario].scn_freq_thermal_ux_max[i] || \
                force_update) { // perfservice might ignore someone's setting before
                maxThermalUxFreqToSet[i] = THERMAL_UX_CPU_MAX_RESET;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        maxThermalUxFreqToSet[i] = max(maxThermalUxFreqToSet[i], ptScnList[j].scn_freq_thermal_ux_max[i]);
                }
                if(maxThermalUxFreqToSet[i] != ptClusterTbl[i].freqThermalUxNow) {
                    ptClusterTbl[i].freqThermalUxNow = maxThermalUxFreqToSet[i];
                    needUpdateThermalUxFreq = 1;
                }
            }
            else {
                maxThermalUxFreqToSet[i] = ptClusterTbl[i].freqThermalUxNow;
            }
            ALOGD("[%s] scn:%s, i:%d, cur_thermal_ux_maxfreq:%d", func_name, scn_name, i, ptClusterTbl[i].freqThermalUxNow);
            //SPD:porting thermal ux policy by sifengtian 20230525 end
        }

        //SPD:porting thermal ux policy by sifengtian 20230525 start
        if(needUpdateThermalUxFreq && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                thermal_freq[cid].max = maxThermalUxFreqToSet[cid];
            }

            if(!needUpdateHardFreq) {
                needUpdateHardFreq = 1;
            }
        }
        //SPD:porting thermal ux policy by sifengtian 20230525 end

        if(needUpdate && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                soft_freq[cid].min = freqToSet[cid];
                soft_freq[cid].max = maxFreqToSet[cid];
                hard_freq[cid].min = hardFreqToSet[cid];
                hard_freq[cid].max = maxHardFreqToSet[cid];
            }
            setClusterFreq(scn_name, nClusterNum, freqToSet, maxFreqToSet);

            if(0 != isMbrainSupport) {
                for (i=0; i<nClusterNum; i++) {
                    notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 1, freqToSet[i]);
                    notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 2, maxFreqToSet[i]);
                }
            }
        }

        if(needUpdateHardFreq && CpuFreqReady == 1) {
            for (int cid = 0; cid < nClusterNum; cid++) {
                soft_freq[cid].min = freqToSet[cid];
                soft_freq[cid].max = maxFreqToSet[cid];
                hard_freq[cid].min = hardFreqToSet[cid];
                hard_freq[cid].max = maxHardFreqToSet[cid];
            }
            setClusterHardFreq(scn_name, nClusterNum, hardFreqToSet, maxHardFreqToSet);

            if(0 != isMbrainSupport) {
                for (i=0; i<nClusterNum; i++) {
                    notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 1, hardFreqToSet[i]);
                    notifyMbrainCap(ptScnList[scenario].pid, ptScnList[scenario].tid, ptScnList[scenario].handle_idx, ptScnList[scenario].lock_duration, ptScnList[scenario].cus_lock_hint, i, 2, maxHardFreqToSet[i]);
                }
            }
        }

        // check core
        //ALOGV("[perfScnUpdate] scenario:%d, scn_cores_now:%d, scn_core_total:%d", scenario, scn_cores_now, ptScnList[scenario].scn_core_total);

        needUpdateCores = 0;
        if (scn_cores_now <= ptScnList[scenario].scn_core_total || force_update) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON)
                    totalCoresToSet = max(totalCoresToSet, ptScnList[i].scn_core_total);
            }

            if (scn_cores_now != totalCoresToSet) {
                ALOGV("[%s] scn_cores_now:%d, totalCoresToSet:%d", func_name, scn_cores_now, totalCoresToSet);
                scn_cores_now = totalCoresToSet;
                needUpdateCores = 1;
            }
        }
        else {
            totalCoresToSet = scn_cores_now;
        }

        for (i=0; i<nClusterNum; i++) {
            lastCore[i] = ptClusterTbl[i].cpuMinNow;
            if (ptClusterTbl[i].cpuMinNow <= ptScnList[scenario].scn_core_min[i] || force_update) {
                coresToSet[i] = -1;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON) {
                        coresToSet[i] = max(coresToSet[i], ptScnList[j].scn_core_min[i]);
                    }
                }
                if(coresToSet[i] != ptClusterTbl[i].cpuMinNow) {
                    ALOGV("[%s] i:%d, cpuMinNow:%d, coresToSet:%d", func_name, i, ptClusterTbl[i].cpuMinNow, coresToSet[i]);
                    ptClusterTbl[i].cpuMinNow = coresToSet[i];
                    needUpdateCores = 1;
                }
            }
            else {
                coresToSet[i] = ptClusterTbl[i].cpuMinNow;
            }

            ALOGV("[%s] scn:%s, i:%d, cpuMaxNow:%d, scn_core_max:%d",
                func_name, scn_name, i, ptClusterTbl[i].cpuMaxNow, ptScnList[scenario].scn_core_max[i]);
            if (ptClusterTbl[i].cpuMaxNow >= ptScnList[scenario].scn_core_max[i] || ptClusterTbl[i].cpuMaxNow == lastCore[i] || force_update) {
                maxCoresToSet[i] = CORE_MAX;
                for (j = 0; j < SCN_APP_RUN_BASE + nPackNum; j++) {
                    if (ptScnList[j].scn_state == STATE_ON)
                        maxCoresToSet[i] = min(maxCoresToSet[i], ptScnList[j].scn_core_max[i]);
                }
                if(maxCoresToSet[i] != ptClusterTbl[i].cpuMaxNow) {
                    ptClusterTbl[i].cpuMaxNow = maxCoresToSet[i];
                    needUpdateCores = 1;
                }
            }
        }

        // fine tune max
        totalCore = scn_cores_now;
        for (i=nClusterNum-1; i>=0; i--) {
            coreToSet = (coresToSet[i] < 0 || coresToSet[i] > ptClusterTbl[i].cpuNum) ? PPM_IGNORE : coresToSet[i];
            if(coreToSet >= 0)
                totalCore -= coreToSet;
            actual_core_min[i] = coreToSet;

            if (ptClusterTbl[i].cpuMaxNow < actual_core_min[i]) { // min priority is higher than max
                ptClusterTbl[i].cpuMaxNow = actual_core_min[i];
                needUpdateCores = 1;
            }
            maxCoresToSet[i] = ptClusterTbl[i].cpuMaxNow;
        }

        /* update core */
        if(needUpdateCores) {
            setClusterCores(scn_name, nClusterNum, scn_cores_now, actual_core_min, maxCoresToSet);
        }

        /*--gpu opp used start--*/
        if ((ptScnList[scenario].scn_gpu_freq != -1) || (ptScnList[scenario].scn_gpu_freq_max != -1) || (ptScnList[scenario].scn_gpu_freq_hard_min != -1) || (ptScnList[scenario].scn_gpu_freq_hard_max != -1)) {
            int baseNeedTraverSal = 0;
            int upBaseNeedTraverSal = 0;
            int baseNeedTraverSal_HL = 0;
            int upBaseNeedTraverSal_HL = 0;
            ALOGI("[unsetGPUFreq] current min:%d, max:%d; scn_gpu_min:%d, scn_gpu_max:%d; scn_gpu_min(HL):%d, scn_gpu_max(HL):%d",
                scn_gpu_freq_now, scn_gpu_freq_max_now,
                ptScnList[scenario].scn_gpu_freq, ptScnList[scenario].scn_gpu_freq_max,
                ptScnList[scenario].scn_gpu_freq_hard_min, ptScnList[scenario].scn_gpu_freq_hard_max);

            lastGpuFreq = scn_gpu_freq_now;
            lastGpuMaxFreq = scn_gpu_freq_max_now;
            lastGpuFreq_HardLimit = scn_hard_gpu_freq_now;
            lastGpuMaxFreq_HardLimit = scn_hard_gpu_freq_max_now;
            /*--GPU opp base was equal to the setting of scenario or--*/
            /*--the setting of scenario don't care the base.        --*/
            /*--We need to choose the min request for base.         --*/
            if((scn_gpu_freq_now == ptScnList[scenario].scn_gpu_freq) ||
                (ptScnList[scenario].scn_gpu_freq != -1) || force_update) {
                    baseNeedTraverSal = 1;
            }
            /*--GPU opp upbound base was equal to the base              --*/
            /*--which might had adjusted by the method (base < upbound )--*/
            /*--the setting of scenario don't care the base.            --*/
            /*--We need to choose the min request for base.             --*/
            if((scn_gpu_freq_max_now == lastGpuFreq) ||
                (scn_gpu_freq_max_now == ptScnList[scenario].scn_gpu_freq_max) ||
                (ptScnList[scenario].scn_gpu_freq_max != -1) || force_update) {
                upBaseNeedTraverSal = 1;
            }

            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {

                    if (baseNeedTraverSal && (ptScnList[i].scn_gpu_freq != -1))
                        gpuFreqToSet = min(gpuFreqToSet, ptScnList[i].scn_gpu_freq);

                    if (upBaseNeedTraverSal && (ptScnList[i].scn_gpu_freq_max != -1))
                        gpuFreqMaxToSet = max(gpuFreqMaxToSet, ptScnList[i].scn_gpu_freq_max);

                 }
             }

             if (baseNeedTraverSal)
                 scn_gpu_freq_now = gpuFreqToSet;

             if (scn_gpu_freq_now != lastGpuFreq || scn_gpu_freq_max_now != lastGpuMaxFreq)
                setGpuFreq(scn_name, scn_gpu_freq_now);

             if (upBaseNeedTraverSal)
                 scn_gpu_freq_max_now = ((gpuFreqMaxToSet > scn_gpu_freq_now) ? (scn_gpu_freq_now) : (gpuFreqMaxToSet));
             else
                 scn_gpu_freq_max_now = ((scn_gpu_freq_max_now > scn_gpu_freq_now) ? (scn_gpu_freq_now) : (scn_gpu_freq_max_now));

             if (scn_gpu_freq_now != lastGpuFreq || scn_gpu_freq_max_now != lastGpuMaxFreq)
                setGpuFreqMax(scn_name, scn_gpu_freq_max_now);

#if GPU_HARDLIMIT_ENABLED
            if ((scn_hard_gpu_freq_now == ptScnList[scenario].scn_gpu_freq_hard_min) || (ptScnList[scenario].scn_gpu_freq_hard_min != -1) || force_update) {
                    baseNeedTraverSal_HL = 1;
            }
            if ((scn_hard_gpu_freq_now == lastGpuFreq_HardLimit) ||
                (scn_hard_gpu_freq_max_now == ptScnList[scenario].scn_gpu_freq_hard_max) ||
                (ptScnList[scenario].scn_gpu_freq_hard_max != -1) ||
                force_update) {
                upBaseNeedTraverSal_HL = 1;
            }

            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    if (baseNeedTraverSal_HL && (ptScnList[i].scn_gpu_freq_hard_min  != -1))
                        gpuFreqToSet_HardLimit = min(gpuFreqToSet_HardLimit, ptScnList[i].scn_gpu_freq_hard_min);
                    if (upBaseNeedTraverSal_HL && (ptScnList[i].scn_gpu_freq_hard_max  != -1))
                        gpuFreqMaxToSet_HardLimit = max(gpuFreqMaxToSet_HardLimit, ptScnList[i].scn_gpu_freq_hard_max);
                }
            }

            if (baseNeedTraverSal_HL)
                scn_hard_gpu_freq_now = gpuFreqToSet_HardLimit;

            if (scn_hard_gpu_freq_now != lastGpuFreq_HardLimit || scn_hard_gpu_freq_max_now != lastGpuMaxFreq_HardLimit)
                setGpuFreq(scn_name, scn_hard_gpu_freq_now );

            if (upBaseNeedTraverSal_HL)
                scn_hard_gpu_freq_max_now = ((gpuFreqMaxToSet_HardLimit > scn_hard_gpu_freq_now ) ? (scn_hard_gpu_freq_now ) : (gpuFreqMaxToSet_HardLimit));
            else
                scn_hard_gpu_freq_max_now = ((scn_hard_gpu_freq_max_now > scn_hard_gpu_freq_now ) ? (scn_hard_gpu_freq_now) : (scn_hard_gpu_freq_max_now));

            if (scn_hard_gpu_freq_now != lastGpuFreq_HardLimit || scn_hard_gpu_freq_max_now != lastGpuMaxFreq_HardLimit)
                setGpuFreqMax(scn_name, scn_hard_gpu_freq_max_now );

            int GPUfreqToSet = scn_gpu_freq_now;
            int GPUFreqMaxToSet = scn_gpu_freq_max_now;
            int final_min_freq = -1, final_max_freq = -1;

            ALOGI("[setGPUFreq] Soft min/max = (%d, %d); Hard min/max = (%d, %d)", scn_gpu_freq_now, scn_gpu_freq_max_now, scn_hard_gpu_freq_now, scn_hard_gpu_freq_max_now);
            if (scn_gpu_freq_now > scn_hard_gpu_freq_now) {
                GPUfreqToSet = scn_hard_gpu_freq_now;
            }
            if (scn_gpu_freq_now < scn_hard_gpu_freq_max_now) {
                GPUfreqToSet = scn_hard_gpu_freq_max_now;
            }
            if (scn_gpu_freq_max_now < scn_hard_gpu_freq_max_now) {
                GPUFreqMaxToSet = scn_hard_gpu_freq_max_now;
            }
            if (scn_gpu_freq_max_now > scn_hard_gpu_freq_now) {
                GPUFreqMaxToSet = scn_hard_gpu_freq_now;
            }
            final_min_freq = GPUfreqToSet;
            final_max_freq = GPUFreqMaxToSet;
            ALOGI("[setGPUFreq] final min/max = (%d, %d)", final_min_freq, final_max_freq);
            setGpuFreq(scn_name, final_min_freq);
            setGpuFreqMax(scn_name, final_max_freq);
#endif
        } /*--gpu opp used end--*/

        for(int idx = 0; idx < FIELD_SIZE; idx++) {
            //if( tConTable[idx].entry.length() == 0 )
            //    break;

            if( tConTable[idx].isValid == -1)
                continue;

            ALOGV("[%s] scn:%s, cmd:%s, reset:%d, default:%d, cur:%d, param:%d", func_name, scn_name, tConTable[idx].cmdName.c_str(),
                tConTable[idx].resetVal, tConTable[idx].defaultVal, tConTable[idx].curVal, ptScnList[scenario].scn_param[idx]);
            if(tConTable[idx].comp.compare(LESS) == 0) {
                if(force_update || (ptScnList[scenario].scn_param[idx] < tConTable[idx].resetVal
                        && ptScnList[scenario].scn_param[idx] <= tConTable[idx].curVal)) {
                    numToSet = tConTable[idx].resetVal;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON) {
                            numToSet = min(numToSet, ptScnList[i].scn_param[idx]);
                        }
                    }

                    numofCurr = tConTable[idx].curVal;
                    tConTable[idx].curVal = numToSet;

                    if(tConTable[idx].curVal != numofCurr) {
                        if(tConTable[idx].curVal == tConTable[idx].resetVal) {
                            if (mode_status[GAME_MODE] == 1) {
                                numToSet = (tConTable[idx].gameVal != CFG_TBL_INVALID_VALUE) ? tConTable[idx].gameVal : tConTable[idx].defaultVal;
                            } else {
                                numToSet = tConTable[idx].defaultVal;
                            }
                        } else {
                            numToSet = tConTable[idx].curVal;
                        }
                        ALOGI("[%s] %s update cmd:%x, param:%d",
                            func_name, scn_name, tConTable[idx].cmdID, numToSet);

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

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON) {
                            numToSet = max(numToSet, ptScnList[i].scn_param[idx]);
                        }
                    }

                    numofCurr = tConTable[idx].curVal;
                    tConTable[idx].curVal = numToSet;

                    if(tConTable[idx].curVal != numofCurr) {
                        if(tConTable[idx].curVal == tConTable[idx].resetVal) {
                            if (mode_status[GAME_MODE] == 1) {
                                numToSet = (tConTable[idx].gameVal != CFG_TBL_INVALID_VALUE) ? tConTable[idx].gameVal : tConTable[idx].defaultVal;
                            } else {
                                numToSet = tConTable[idx].defaultVal;
                            }
                        } else {
                            numToSet = tConTable[idx].curVal;
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d",
                            func_name, scn_name, tConTable[idx].cmdID, numToSet);
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

            ALOGV("[%s] RscCtlTbl[%d] scn:%s, cmd:%s, reset:%d, default:%d, cur:%d, param:%d",
                func_name, idx, scn_name, RscCfgTbl[idx].cmdName.c_str(), gRscCtlTbl[idx].resetVal,
                RscCfgTbl[idx].defaultVal, gRscCtlTbl[idx].curVal, ptScnList[scenario].scn_rsc[idx]);

            if (RscCfgTbl[idx].comp == SMALLEST) {
                if(force_update || (ptScnList[scenario].scn_rsc[idx] < gRscCtlTbl[idx].resetVal
                        && ptScnList[scenario].scn_rsc[idx] <= gRscCtlTbl[idx].curVal)) {
                    numToSet = gRscCtlTbl[idx].resetVal;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON) {
                            numToSet = min(numToSet, ptScnList[i].scn_rsc[idx]);
                        }
                    }

                    numofCurr = gRscCtlTbl[idx].curVal;
                    gRscCtlTbl[idx].curVal = numToSet;

                    if(gRscCtlTbl[idx].curVal != numofCurr || (RscCfgTbl[idx].force_update == 1 && gRscCtlTbl[idx].curVal != gRscCtlTbl[idx].resetVal)) {
                        if(gRscCtlTbl[idx].curVal == gRscCtlTbl[idx].resetVal) {
                            numToSet = RscCfgTbl[idx].defaultVal;
                            if (mode_status[GAME_MODE] == 1) {
                                numToSet = (RscCfgTbl[idx].gameVal != RSC_TBL_INVALID_VALUE) ? RscCfgTbl[idx].gameVal : RscCfgTbl[idx].defaultVal;
                            }
                            result = RscCfgTbl[idx].unset_func(numToSet, (void*)&ptScnList[scenario]);
                        } else {
                            numToSet = gRscCtlTbl[idx].curVal;
                            result = RscCfgTbl[idx].set_func(numToSet, (void*)&ptScnList[scenario]);
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d",
                            func_name, scn_name, RscCfgTbl[idx].cmdID, numToSet);
                    }
                    ALOGV("[%s] RscCfgTbl SMALLEST numToSet:%d, ret:%d", func_name, numToSet, result);
                }
            } else if (RscCfgTbl[idx].comp == BIGGEST) {
                if(force_update || (ptScnList[scenario].scn_rsc[idx] > gRscCtlTbl[idx].resetVal
                        && ptScnList[scenario].scn_rsc[idx] >= gRscCtlTbl[idx].curVal)) {
                    numToSet = gRscCtlTbl[idx].resetVal;

                    for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                        if (ptScnList[i].scn_state == STATE_ON) {
                            numToSet = max(numToSet, ptScnList[i].scn_rsc[idx]);
                        }
                    }

                    numofCurr = gRscCtlTbl[idx].curVal;
                    gRscCtlTbl[idx].curVal = numToSet;

                    if(gRscCtlTbl[idx].curVal != numofCurr || (RscCfgTbl[idx].force_update == 1 && gRscCtlTbl[idx].curVal != gRscCtlTbl[idx].resetVal)) {
                        if(gRscCtlTbl[idx].curVal == gRscCtlTbl[idx].resetVal) {
                            numToSet = RscCfgTbl[idx].defaultVal;
                            if (mode_status[GAME_MODE] == 1) {
                                numToSet = (RscCfgTbl[idx].gameVal != RSC_TBL_INVALID_VALUE) ? RscCfgTbl[idx].gameVal : RscCfgTbl[idx].defaultVal;
                            }
                            result = RscCfgTbl[idx].unset_func(numToSet, (void*)&ptScnList[scenario]);
                        } else {
                            numToSet = gRscCtlTbl[idx].curVal;
                            result = RscCfgTbl[idx].set_func(numToSet, (void*)&ptScnList[scenario]);
                        }
                        ALOGI("[%s] %s update cmd:%x param:%d",
                            func_name, scn_name, RscCfgTbl[idx].cmdID, numToSet);
                    }
                    ALOGV("[%s] RscCfgTbl BIGGEST numToSet:%d, ret:%d", func_name, numToSet, result);
                }
            } else {
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
                        ptScnList[scenario].scn_rsc[idx] != ptScnList[scenario].scn_prev_rsc[idx]) \
                        || RscCfgTbl[idx].force_update == 1) {
                        result = RscCfgTbl[idx].set_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                        ALOGV("[%s] RscCfgTbl ONESHOT cur:%d, ret:%d", func_name, gRscCtlTbl[idx].curVal, result);
                    }
                } else if(ptScnList[scenario].scn_rsc[idx] != gRscCtlTbl[idx].resetVal) {
                    result = RscCfgTbl[idx].unset_func(ptScnList[scenario].scn_rsc[idx], (void*)&ptScnList[scenario]);
                    ALOGI("[%s] %s update ONESHOT(unset) cmd:%x param:%d",
                        func_name, scn_name, RscCfgTbl[idx].cmdID, ptScnList[scenario].scn_rsc[idx]);

                    if (mode_status[GAME_MODE] == 1) {
                        if (RscCfgTbl[idx].gameVal != RSC_TBL_INVALID_VALUE) {
                            result = RscCfgTbl[idx].set_func(RscCfgTbl[idx].gameVal, (void*)&ptScnList[scenario]);
                            ALOGI("[%s] %s update ONESHOT(set) cmd:%x param:%d", func_name, scn_name, RscCfgTbl[idx].cmdID, RscCfgTbl[idx].gameVal);
                        }
                    }
                }
            }
        }
    }

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
                for (j = 0; j < nClusterNum; j++)
                    ALOGI("%s - freq:%d, %d, HL:%d, %d",
                        scn_name,
                        ptScnList[i].scn_freq_min[j], (ptScnList[i].scn_freq_max[j] == FREQ_MAX) ? -1 : ptScnList[i].scn_freq_max[j],
                        ptScnList[i].scn_freq_hard_min[j], (ptScnList[i].scn_freq_hard_max[j] == FREQ_MAX) ? -1 : ptScnList[i].scn_freq_hard_max[j]);
                ALOGI("%s - gpu freq min:%d, max:%d, scn_action:%d",
                    scn_name, ptScnList[i].scn_gpu_freq, ptScnList[i].scn_gpu_freq_max, ptScnList[i].screen_off_action);

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

    case DUMP_CPU_CTL:
        // check all user
        for (i = 0; i < SCN_APP_RUN_BASE+nPackNum; i++) {
            if (ptScnList[i].scn_state == STATE_ON) {
                GetScnName(i, "perfScnDumpAll", scn_name);
                for (j = 0; j < nClusterNum; j++) {
                    ALOGI("%s - cpu:%d, freq:%d", scn_name, ptScnList[i].scn_core_min[j], ptScnList[i].scn_freq_min[j]);
                }
            }
        }
        break;

    case DUMP_GPU_CTL:
        // check all user
        for (i = 0; i < SCN_APP_RUN_BASE+nPackNum; i++) {
            if (ptScnList[i].scn_state == STATE_ON) {
                GetScnName(i, "perfScnDumpAll", scn_name);
                ALOGI("%s - gpu freq min:%d max:%d", scn_name, ptScnList[i].scn_gpu_freq, ptScnList[i].scn_gpu_freq_max);
            }
        }
        break;

    case DUMP_ALL_USER:
        // check all perflock user
        for (i = nUserScnBase; i < nUserScnBase + REG_SCN_MAX; i++) {
            if (ptScnList[i].pid != -1)
                ALOGI("[perfScnDumpAll] scn:%d user_name:%s pid:%d tid:%d state:%d type:%d hdl:%d hint:%d",
                    i, ptScnList[i].comm, ptScnList[i].pid, ptScnList[i].tid, ptScnList[i].scn_state,
                    ptScnList[i].scn_type, ptScnList[i].handle_idx, ptScnList[i].cus_lock_hint);
        }
        break;

    case DUMP_APP_CFG:
        ALOGI("nPackNum:%d", nPackNum);
        for (i = SCN_APP_RUN_BASE; i < SCN_APP_RUN_BASE + nPackNum; i++) {
            ALOGI("pack:%s, act:%s, fps:%s", ptScnList[i].pack_name, ptScnList[i].act_name, ptScnList[i].fps);
            char str[128];
            for (j = 0; j < nClusterNum; j++) {
                char buf[32];
                if(sprintf(buf, "%d %d ", ptScnList[i].scn_freq_min[j], (ptScnList[i].scn_freq_max[j] == FREQ_MAX) ? -1 : ptScnList[i].scn_freq_max[j]) < 0) {
                    ALOGE("sprintf error");
                }
                strncat(str, buf, strlen(buf));
            }
            ALOGI("   cpu freq: %s", str);
            ALOGI("   gpu freq: %d %d", ptScnList[i].scn_gpu_freq, ptScnList[i].scn_gpu_freq_max);

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

            usleep(10000);
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
                else if (cmd == PERF_RES_POWERHAL_SPORTS_MODE) {
                    ALOGI("%s - SPORTS_MODE param:%d", scn_name, ptScnList[i].sports_mode);
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
        ptScnList[handle].priority = PRIORITY_HIGHEST;
    }
    ptScnList[handle].scn_core_total    = 0;
    ptScnList[handle].scn_gpu_freq      = GPU_FREQ_MIN_RESET;
    ptScnList[handle].scn_gpu_freq_max  = GPU_FREQ_MAX_RESET;
    ptScnList[handle].scn_gpu_freq_hard_min = GPU_FREQ_MIN_RESET;
    ptScnList[handle].scn_gpu_freq_hard_max = GPU_FREQ_MAX_RESET;
    ptScnList[handle].screen_off_action = MTKPOWER_SCREEN_OFF_DISABLE;
    ptScnList[handle].render_pid = -1;

    for (i=0; i<nClusterNum; i++) {
        ptScnList[handle].scn_core_min[i] = CPU_CORE_MIN_RESET;
        ptScnList[handle].scn_core_max[i] = CPU_CORE_MAX_RESET;
        ptScnList[handle].scn_freq_min[i] = CPU_FREQ_MIN_RESET;
        ptScnList[handle].scn_freq_max[i] = CPU_FREQ_MAX_RESET;
        ptScnList[handle].scn_freq_hard_min[i] = CPU_FREQ_MIN_RESET;
        ptScnList[handle].scn_freq_hard_max[i] = CPU_FREQ_MAX_RESET;
        //SPD:porting thermal ux policy by sifengtian 20230525 start
        ptScnList[handle].scn_freq_thermal_ux_max[i] = THERMAL_UX_CPU_MAX_RESET;
        //SPD:porting thermal ux policy by sifengtian 20230525 end
    }

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

    if (((icmd == -1) && !strcmp(scmd, "PERF_RES_GPU_FREQ_MIN")) ||
            icmd == PERF_RES_GPU_FREQ_MIN) {
        scenario->scn_gpu_freq =
            (param_1 >= 0 && param_1 <= nGpuHighestFreqLevel) ? param_1 : nGpuHighestFreqLevel;
        icmd = PERF_RES_GPU_FREQ_MIN;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_GPU_FREQ_MAX")) ||
            icmd == PERF_RES_GPU_FREQ_MAX) {
        scenario->scn_gpu_freq_max =
            (param_1 >= 0 && param_1 <= nGpuHighestFreqLevel) ? param_1 : nGpuHighestFreqLevel;
        icmd = PERF_RES_GPU_FREQ_MAX;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_GPU_FREQ_MIN_HL")) ||
            icmd == PERF_RES_GPU_FREQ_MIN_HL) {
        scenario->scn_gpu_freq_hard_min =
            (param_1 >= 0 && param_1 <= nGpuHighestFreqLevel) ? param_1 : nGpuHighestFreqLevel;
        icmd = PERF_RES_GPU_FREQ_MIN_HL;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_GPU_FREQ_MAX_HL")) ||
            icmd == PERF_RES_GPU_FREQ_MAX_HL) {
        scenario->scn_gpu_freq_hard_max =
            (param_1 >= 0 && param_1 <= nGpuHighestFreqLevel) ? param_1 : nGpuHighestFreqLevel;
        icmd = PERF_RES_GPU_FREQ_MAX_HL;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MIN_CLUSTER_0")) ||
            icmd == PERF_RES_CPUCORE_MIN_CLUSTER_0) {
        if (nClusterNum > 0) {
            scenario->scn_core_min[0] =
                (param_1 <= ptClusterTbl[0].cpuNum) ?
                ((param_1 < 0) ? 0 : param_1) : ptClusterTbl[0].cpuNum;
        }

        scenario->scn_core_total = 0;
        for (i=0; i<nClusterNum; i++) {
            if (scenario->scn_core_min[i] >= 0 && scenario->scn_core_min[i] <= ptClusterTbl[i].cpuNum)
                scenario->scn_core_total += scenario->scn_core_min[i];
        }
        icmd = PERF_RES_CPUCORE_MIN_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MIN_CLUSTER_1")) ||
            icmd == PERF_RES_CPUCORE_MIN_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_core_min[1] =
                (param_1 <= ptClusterTbl[1].cpuNum) ?
                ((param_1 < 0) ? 0 : param_1) : ptClusterTbl[1].cpuNum;

            scenario->scn_core_total = 0;
            for (i=0; i<nClusterNum; i++) {
                if (scenario->scn_core_min[i] >= 0 && scenario->scn_core_min[i] <= ptClusterTbl[i].cpuNum)
                    scenario->scn_core_total += scenario->scn_core_min[i];
            }
        }
        icmd = PERF_RES_CPUCORE_MIN_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MIN_CLUSTER_2")) ||
            icmd == PERF_RES_CPUCORE_MIN_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_core_min[2] =
                (param_1 <= ptClusterTbl[2].cpuNum) ?
                ((param_1 < 0) ? 0 : param_1) : ptClusterTbl[2].cpuNum;

            scenario->scn_core_total = 0;
            for (i=0; i<nClusterNum; i++) {
                if (scenario->scn_core_min[i] >= 0 && scenario->scn_core_min[i] <= ptClusterTbl[i].cpuNum)
                    scenario->scn_core_total += scenario->scn_core_min[i];
            }
        }
        icmd = PERF_RES_CPUCORE_MIN_CLUSTER_2;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MAX_CLUSTER_0")) ||
            icmd == PERF_RES_CPUCORE_MAX_CLUSTER_0) {
        if (nClusterNum > 0) {
            scenario->scn_core_max[0] = (param_1 >= 0) ?
                param_1 : ptClusterTbl[0].cpuNum;
        }
        icmd = PERF_RES_CPUCORE_MAX_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MAX_CLUSTER_1")) ||
            icmd == PERF_RES_CPUCORE_MAX_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_core_max[1] = (param_1 >= 0) ?
                param_1 : ptClusterTbl[1].cpuNum;
        }
        icmd = PERF_RES_CPUCORE_MAX_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUCORE_MAX_CLUSTER_2")) ||
            icmd == PERF_RES_CPUCORE_MAX_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_core_max[2] = (param_1 >= 0) ?
                param_1 : ptClusterTbl[2].cpuNum;
        }
        icmd = PERF_RES_CPUCORE_MAX_CLUSTER_2;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_CLUSTER_0")) ||
            icmd == PERF_RES_CPUFREQ_MIN_CLUSTER_0) {
        if (nClusterNum > 0) {
            scenario->scn_freq_min[0] =
                (param_1 >= ptClusterTbl[0].freqMax) ? ptClusterTbl[0].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_CLUSTER_1")) ||
            icmd == PERF_RES_CPUFREQ_MIN_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_freq_min[1] =
                (param_1 >= ptClusterTbl[1].freqMax) ? ptClusterTbl[1].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_CLUSTER_2")) ||
            icmd == PERF_RES_CPUFREQ_MIN_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_freq_min[2] =
                (param_1 >= ptClusterTbl[2].freqMax) ? ptClusterTbl[2].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_CLUSTER_2;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_CLUSTER_0")) ||
            icmd == PERF_RES_CPUFREQ_MAX_CLUSTER_0) {
        if (nClusterNum > 0) {
            scenario->scn_freq_max[0] =
                (param_1 >= ptClusterTbl[0].freqMax) ? ptClusterTbl[0].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_CLUSTER_1")) ||
            icmd == PERF_RES_CPUFREQ_MAX_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_freq_max[1] =
                (param_1 >= ptClusterTbl[1].freqMax) ? ptClusterTbl[1].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_CLUSTER_2")) ||
            icmd == PERF_RES_CPUFREQ_MAX_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_freq_max[2] =
                (param_1 >= ptClusterTbl[2].freqMax) ? ptClusterTbl[2].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_CLUSTER_2;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_FPS_FPSGO_RENDER_PID")) ||
            icmd == PERF_RES_FPS_FPSGO_RENDER_PID) {
        scenario->render_pid = param_1;
        icmd = PERF_RES_FPS_FPSGO_RENDER_PID;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_PERF_MODE")) ||
            icmd == PERF_RES_CPUFREQ_PERF_MODE) {
        if(param_1 != 1) // param_1 must be 1
            return 0;

        for (i = 0; i < nClusterNum; i++) {
            //ALOGI("cmdSetting - cmd:%x, i:%d, cpu:%d, freq:%d", icmd, i, ptClusterTbl[i].cpuNum, ptClusterTbl[i].freqMax);
            scenario->scn_core_min[i] = ptClusterTbl[i].cpuNum;
            scenario->scn_core_total += scenario->scn_core_min[i];
            scenario->scn_freq_min[i] = ptClusterTbl[i].freqMax;
        }
        icmd = PERF_RES_CPUFREQ_PERF_MODE;
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
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0")) ||
            icmd == PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0) {
        if(nClusterNum > 0) {
            scenario->scn_freq_hard_min[0] =
                (param_1 >= ptClusterTbl[0].freqMax) ? ptClusterTbl[0].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_HL_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1")) ||
            icmd == PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_freq_hard_min[1] =
                (param_1 >= ptClusterTbl[1].freqMax) ? ptClusterTbl[1].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_HL_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2")) ||
            icmd == PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_freq_hard_min[2] =
                (param_1 >= ptClusterTbl[2].freqMax) ? ptClusterTbl[2].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MIN_HL_CLUSTER_2;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0")) ||
            icmd == PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0) {
        if(nClusterNum > 0) {
            scenario->scn_freq_hard_max[0] =
                (param_1 >= ptClusterTbl[0].freqMax) ? ptClusterTbl[0].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_HL_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1")) ||
            icmd == PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_freq_hard_max[1] =
                (param_1 >= ptClusterTbl[1].freqMax) ? ptClusterTbl[1].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_HL_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2")) ||
            icmd == PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_freq_hard_max[2] =
                (param_1 >= ptClusterTbl[2].freqMax) ? ptClusterTbl[2].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_HL_CLUSTER_2;
    }else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_POWERHAL_PRIORITY")) ||
            icmd == PERF_RES_POWERHAL_PRIORITY) {
            scenario->priority = param_1;
            icmd = PERF_RES_POWERHAL_PRIORITY;
    }
    //SPD:porting thermal ux policy by sifengtian 20230525 start
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_0")) ||
            icmd == PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_0) {
        if(nClusterNum > 0) {
            scenario->scn_freq_thermal_ux_max[0] =
                (param_1 >= ptClusterTbl[0].freqMax) ? ptClusterTbl[0].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_0;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_1")) ||
            icmd == PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_1) {
        if(nClusterNum > 1) {
            scenario->scn_freq_thermal_ux_max[1] =
                (param_1 >= ptClusterTbl[1].freqMax) ? ptClusterTbl[1].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_1;
    }
    else if (((icmd == -1) && !strcmp(scmd, "PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_2")) ||
            icmd == PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_2) {
        if(nClusterNum > 2) {
            scenario->scn_freq_thermal_ux_max[2] =
                (param_1 >= ptClusterTbl[2].freqMax) ? ptClusterTbl[2].freqMax : param_1;
        }
        icmd = PERF_RES_CPUFREQ_MAX_THERMAL_CLUSTER_2;
    }
    //SPD:porting thermal ux policy by sifengtian 20230525 end
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
            ALOGD("cmdSetting RscCfgTbl cmdID:%x param_1:%d ,maxVal:%d ,minVal:%d, force:%d", RscCfgTbl[idx].cmdID, param_1, RscCfgTbl[idx].maxVal,
                  RscCfgTbl[idx].minVal, RscCfgTbl[idx].force_update);
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

            if(RscCfgTbl[idx].force_update == 1)
                ret = 1;
            else
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
    int rsc, size;
    tScnNode tmp;
    tmp.scn_core_total = 0;
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

int updateDefaultValue(int enable)
{
    int i = 0, idx = 0, numToSet = 0, result = 0, priority;
    LOG_D("enable: %d", enable);

    for (idx = 0; idx < FIELD_SIZE; idx++) {
        if (tConTable[idx].isValid == -1 || tConTable[idx].cmdID == 0)
            continue;

        numToSet = tConTable[idx].resetVal;
        priority = PRIORITY_LOWEST;

        if (tConTable[idx].comp.compare(LESS) == 0) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    numToSet = min(numToSet, ptScnList[i].scn_param[idx]);
                    priority = min(priority, ptScnList[i].priority);
                }
            }
        } else if (tConTable[idx].comp.compare(MORE) == 0) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    numToSet = max(numToSet, ptScnList[i].scn_param[idx]);
                    priority = min(priority, ptScnList[i].priority);
                }
            }
        }

        LOG_D("game priority: %d", priority);

        if (numToSet == tConTable[idx].resetVal || priority != PRIORITY_HIGHEST) {
            // no one sets this resource or priority is same as game mode
            if (tConTable[idx].gameVal == CFG_TBL_INVALID_VALUE)
                continue;

            if (mode_status[GAME_MODE] == 1) {
                numToSet = tConTable[idx].gameVal;
            } else if (mode_status[GAME_MODE] == 0) {
                numToSet = tConTable[idx].defaultVal;
            }

            if (tConTable[idx].entry.length() > 0) {
                if (tConTable[idx].prefix.length() == 0) {
                    set_value(tConTable[idx].entry.c_str(), numToSet);
                } else {
                    char inBuf[64];
                    if (snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), numToSet) < 0) {
                        ALOGE("snprint error");
                    }
                    if (tConTable[idx].entry.length() > 0) {
                        set_value(tConTable[idx].entry.c_str(), inBuf);
                    }
                }
            }
            LOG_I("[%s] update cmd:%x param:%d",
                (mode_status[GAME_MODE] == 1) ? "Game_Mode" : "normal_mode", tConTable[idx].cmdID, numToSet);
        }

    }

    for (idx = 0; idx < gRscCtlTblLen; idx++) {
        if (RscCfgTbl[idx].set_func == NULL || RscCfgTbl[idx].unset_func == NULL || gRscCtlTbl[idx].isValid != 1)
            continue;

        numToSet = gRscCtlTbl[idx].resetVal;
        priority = PRIORITY_LOWEST;

        if (RscCfgTbl[idx].comp == SMALLEST) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    numToSet = min(numToSet, ptScnList[i].scn_rsc[idx]);
                    priority = min(priority, ptScnList[i].priority);
                }
            }
        } else if (RscCfgTbl[idx].comp == BIGGEST) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    numToSet = max(numToSet, ptScnList[i].scn_rsc[idx]);
                    priority = min(priority, ptScnList[i].priority);
                }
            }
        } else if (RscCfgTbl[idx].comp == ONESHOT) {
            for (i = 0; i < SCN_APP_RUN_BASE + nPackNum; i++) {
                if (ptScnList[i].scn_state == STATE_ON) {
                    if (ptScnList[i].scn_rsc[idx] != gRscCtlTbl[idx].resetVal) {
                        numToSet = ptScnList[i].scn_rsc[idx];
                        priority = min(priority, ptScnList[i].priority);
                    }
                }
            }
        }

        if (numToSet == gRscCtlTbl[idx].resetVal || priority != PRIORITY_HIGHEST) {
            // no one sets this resource
            if (RscCfgTbl[idx].gameVal == RSC_TBL_INVALID_VALUE)
                continue;

            if (mode_status[GAME_MODE] == 1) {
                numToSet = RscCfgTbl[idx].gameVal;
            } else if (mode_status[GAME_MODE] == 0) {
                numToSet = (RscCfgTbl[idx].comp == ONESHOT) ? RscCfgTbl[idx].gameVal : RscCfgTbl[idx].defaultVal;
            }

            if (enable == 1) {
                result = RscCfgTbl[idx].set_func(numToSet, NULL);
            } else {
                result = RscCfgTbl[idx].unset_func(numToSet, NULL);
            }
            if (RscCfgTbl[idx].comp == ONESHOT) {
                ALOGI("[%s] update ONESHOT(%s) cmd:%x param:%d",
                    (mode_status[GAME_MODE] == 1) ? "Game_Mode" : "normal_mode",
                    (enable == 1) ? "set" : "unset",
                    RscCfgTbl[idx].cmdID, numToSet);
            } else {
                LOG_I("[%s] update cmd:%x param:%d",
                    (mode_status[GAME_MODE] == 1) ? "Game_Mode" : "normal_mode",
                    RscCfgTbl[idx].cmdID, numToSet);
            }
        }
    }

    return 0;
}

int init_GameMode()
{
    int *rsc_list, size = 0, i = 0, j = 0;

    if (getHintRscSize(MTKPOWER_HINT_GAME_MODE) == -1) {
        return 0;
    }

    if ((rsc_list = (int*)malloc(sizeof(int)*getHintRscSize(MTKPOWER_HINT_GAME_MODE))) == NULL) {
        ALOGE("Cannot allocate memory");
        return 0;
    }

    size = getHintRscSize(MTKPOWER_HINT_GAME_MODE);
    if (size <= 0) {
        free(rsc_list);
        return 0;
    }

    if (rsc_list != NULL) {
        if (getHintRscList(MTKPOWER_HINT_GAME_MODE) != nullptr)
            memcpy(rsc_list, getHintRscList(MTKPOWER_HINT_GAME_MODE), sizeof(int)*(size));
        else {
            free(rsc_list);
            return 0;
        }
    }

    for (i = 0; i < size; i = i + 2) {
        for (j = 0; j < FIELD_SIZE; j ++) {
            if (rsc_list[i] == tConTable[j].cmdID) {
                tConTable[j].gameVal = rsc_list[i+1];
                LOG_I("%x %d", tConTable[j].cmdID, tConTable[j].gameVal);
            }
        }
        for (j = 0; j < gRscCtlTblLen; j ++) {
            if (rsc_list[i] == RscCfgTbl[j].cmdID) {
                RscCfgTbl[j].gameVal = rsc_list[i+1];
                LOG_I("%x %d", RscCfgTbl[j].cmdID, RscCfgTbl[j].gameVal);
            }
        }
    }

    free(rsc_list);

    return 0;
}

int setGameMode(int enable, void *scn)
{
    if(mode_status[GAME_MODE] == 0) {
        notifyMBrainGameMode(1);
    }
    mode_status[GAME_MODE] = 1;
    LOG_I("enable: %d", mode_status[GAME_MODE]);
    updateDefaultValue(mode_status[GAME_MODE]);

    return 1;
}

int disableGameMode(int enable, void *scn)
{
    if(mode_status[GAME_MODE] == 1) {
        notifyMBrainGameMode(0);
    }
    mode_status[GAME_MODE] = 0;
    LOG_I("enable: %d", mode_status[GAME_MODE]);
    updateDefaultValue(mode_status[GAME_MODE]);

    return 1;
}

void dumpPowerhalMode()
{
    int i = 0;

    for (i = 0; i < MODE_COUNT; i ++)
        LOG_I("mode_status[%d] = %d", i ,mode_status[i]);
}

int queryInteractiveState()
{
    LOG_D("%d", mInteractive);
    return mInteractive;
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
    //BSP:add for thermal policy switch by jian.li at 20240424 start
    if (state == FPS_UPDATED || state == STATE_RESUMED) {
        g_current_paused_pack[0] = '\0';
        strncpy(g_current_foreground_pack, packName, PACK_NAME_MAX-1);
    } else if (state == STATE_PAUSED) {
        strncpy(g_current_paused_pack, packName, PACK_NAME_MAX-1);
    }
    //BSP:add for thermal policy switchby jian.li at 20240424 end
    LOG_I("not supported");
    return 0;
}

int perfUserScnDisableAll(void)
{
    int i;
    struct stat stat_buf;
    int exist;
    char proc_path[128];

    Mutex::Autolock lock(sMutex);
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
                    if(sprintf(proc_path, "/proc/%d", ptScnList[i].pid) < 0) {
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

    Mutex::Autolock lock(sMutex);
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

    Mutex::Autolock lock(sMutex);
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
                    if(sprintf(proc_path, "/proc/%d", ptScnList[i].pid) < 0) {
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
    Mutex::Autolock lock(sMutex);
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
        game_mode_pid = atoi(data);

        if ((notifyGbePid == 1) && (set_value(PATH_GBE_PID, game_mode_pid) == 0))
            notifyGbePid = 0;

        break;

    case SETSYS_VIDEO_MODE_PID:
        LOG_D("type:%d, %s", type, data);
        video_mode_pid = atoi(data);
        break;

    case SETSYS_CAMERA_MODE_PID:
        LOG_D("type:%d, %s", type, data);
        camera_mode_pid = atoi(data);
        break;

    case SETSYS_NETD_SET_BOOST_UID:
        LOG_D("type:%d, %s", type, data);
        foreground_app_info.uid = atoi(data);
        break;

    case SETSYS_FOREGROUND_APP_PID:
        LOG_I("pid type:%d, %s", type, data);
        foreground_app_info.pid = atoi(data);
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
                uid = atoi(str);
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
           	uid = atoi(str);
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

    case SETSYS_SPORTS_APK:{
        char newBuf[1024] = {0};
        LOG_D("SETSYS_SPORTS_APK, data:%s", data);
        strncpy(newBuf, data, sizeof(newBuf)-1);
        str = strtok_r(newBuf, "|", &saveptr);
        LOG_D("SETSYS_SPORTS_APK newBuf:%s str:%p", newBuf, str);
        if(str != NULL) {
           char *strState = str;
           int state = -1;
           int fps  = 0;
           if (strState != NULL){
               state = atoi(strState);
           }
           char *srcFPS = strtok_r(NULL, "|", &saveptr);
           if (srcFPS != NULL){
              fps = atoi(srcFPS);
           }
           char *srcPackName = strtok_r(NULL, "|", &saveptr);
           //BSP:add for thermal policy switch by jian.li at 20240424 start
            if (state == FPS_UPDATED || state == STATE_RESUMED) {
                if (srcPackName != NULL && strncmp(srcPackName, "com.transsion.smartpanel", strlen("com.transsion.smartpanel")) != 0
                    && strncmp(srcPackName, "com.transsion.multiwindow", strlen("com.transsion.multiwindow")) != 0
                    && strnlen(srcPackName, 2) > 0) {
                    g_current_paused_pack[0] = '\0';
                    g_current_foreground_pack[0] = '\0';
                    strncpy(g_current_foreground_pack, srcPackName, PACK_NAME_MAX-1);
                }
            } else if (state == STATE_PAUSED) {
                if (srcPackName != NULL && strncmp(srcPackName, "com.transsion.smartpanel", strlen("com.transsion.smartpanel")) != 0
                    && strncmp(srcPackName, "com.transsion.multiwindow", strlen("com.transsion.multiwindow")) != 0
                    && strnlen(srcPackName, 2) > 0) {
                    strncpy(g_current_paused_pack, srcPackName, PACK_NAME_MAX-1);
                }
            }
            LOG_D("state:%d, g_current_foreground_pack:%s, g_current_paused_pack:%s", state, g_current_foreground_pack, g_current_paused_pack);
            //BSP:add for thermal policy switch by jian.li at 20240424 end

            /* foreground change: update pack name */
            if((state == STATE_RESUMED && strcmp(foreground_app_info.packName, srcPackName)) || state == FPS_UPDATED) {
                strncpy(foreground_app_info.packName, srcPackName, PACK_NAME_MAX-1); // update pack name

                if (state != FPS_UPDATED)
                    LOG_D("foreground:%s", foreground_app_info.packName);
            }
        }
        break;
    }
    case SETSYS_FOREGROUND_SPORTS:
    case SETSYS_INTERNET_STATUS:
        LOG_E("not support type:%d", type);
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

    for (i=0; i<vEaraCallback.size(); i++) {
        if (vEaraCallback[i].scn_set == scenario_set && vEaraCallback[i].callback == callback)
            return 0;
    }

    LOG_I("[%s]set scn:%x", __FUNCTION__, scenario_set);
    tCb.scn_set = scenario_set;
    tCb.last_sent_scn = -1;
    tCb.callback = callback;

    pthread_mutex_lock(&vec_mutex);
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

    LOG_I("[%s]scn:%x , vEaraCallback.size(%d)", __FUNCTION__, scn, vEaraCallback.size());


    pthread_mutex_lock(&vec_mutex);

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
    for (i=0; i<vInvalidCb.size(); i++) {
        LOG_I("[%s]invalid callback:%d", __FUNCTION__, vInvalidCb[i]);
        vEaraCallback.erase(vEaraCallback.begin() + vInvalidCb[i]);
    }

    pthread_mutex_unlock(&vec_mutex);

    //last_scn = scn;
    return 0;
}

int perfDumpAll(int cmd)
{
    Mutex::Autolock lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;
    return perfScnDumpAll(cmd);
}

int perfUserGetCapability(int cmd, int id)
{
    int value = -1, idx = -1, i = 0;
    nsecs_t now = 0;
    int interval = -1;

    Mutex::Autolock lock(sMutex);
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
        dumpPowerhalMode();
        break;

    case MTKPOWER_CMD_GET_API_STATUS:
        value = queryAPIstatus(id);
        LOG_I("API: %d, enabled: %d", id, value);
        break;

    case 300:
        value = xgfGetFstbActive(100000);
        LOG_I("xgfGetFstbActive: %d", value);
        break;

    case 301:
        value = xgfWaitFstbActive();
        LOG_I("xgfWaitFstbActive: %d", value);
        break;

    case MTKPOWER_CMD_GET_TOUCH_BOOST_STATUS:
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
    if(sprintf(proc_path, "/proc/%d", pid) < 0) {
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
                            if(sprintf(proc_path, "/proc/%d", ptScnList[i].pid) < 0) {
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

    Mutex::Autolock lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    return perfLockCheckHdl_internal(handle, hint, size, pid, tid, duration, &index);
}

//SPF:fix tiktok invalid acquire for X6856OS151-2711 by sifeng.tian 20250224 start
static bool check_invalid_acq(int pid, int tid, int hint, int idx)
{
    int i;
    char pid_path[64] = "\0";
    char tid_path[64] = "\0";
    char pid_comm[64] = "\0";
    char tid_comm[64] = "\0";

    if (hint != -1) {
        LOG_D("hint:%d is not -1", hint);
        return false;
    }

    if (strcmp(foreground_app_info.packName, "com.ss.android.ugc.trill")) {
        LOG_V("foreground pack is not com.ss.android.ugc.trill");
        return false;
    }
    if(snprintf(pid_path, sizeof(pid_path), "/proc/%d/comm", pid) < 0
        || snprintf(tid_path, sizeof(tid_path), "/proc/%d/comm", tid) < 0) {
        LOG_E("snprint error");
        return false;
    }

    get_task_comm(pid_path, pid_comm);
    get_task_comm(tid_path, tid_comm);
    LOG_V("pid_comm:%s tid_comm:%s", pid_comm, tid_comm);
    if (strcmp(pid_comm, "system_server") || strstr(tid_comm, "binder:") == NULL) {
        LOG_V("pid_comm is not system_server or tid_comm is not binder");
        return false;
    }

    LOG_V("scn min_0:%d min_1:%d min_2:%d max_0:%d max_1:%d max_2:%d",
        ptScnList[idx].scn_freq_min[0], ptScnList[idx].scn_freq_min[1], ptScnList[idx].scn_freq_min[2],
        ptClusterTbl[0].freqMax, ptClusterTbl[1].freqMax, ptClusterTbl[2].freqMax);
    if(ptScnList[idx].scn_freq_min[0] == ptClusterTbl[0].freqMax
        && ptScnList[idx].scn_freq_min[1] == ptClusterTbl[1].freqMax
        && ptScnList[idx].scn_freq_min[2] == ptClusterTbl[2].freqMax) {
        LOG_I("invalid freq acquire");
        return true;
    }

    return false;
}
//SPF:fix tiktok invalid acquire for X6856OS151-2711 by sifeng.tian 20250224 end

int perfLockAcq(int *list, int handle, int size, int pid, int tid, int duration, int hint)
{
    int i, idx = -1;
    int cmd, value, ret_handle;
    int hdl_enabled = 0, rsc_modified = 0, force_update = 0;
    int log_hasI = 1;

    Mutex::Autolock lock(sMutex);
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

    if (hint == MTKPOWER_HINT_GAME_MODE) {
        LOG_D("ptScnList[%d].render_pid = %d", idx, game_mode_pid);
        ptScnList[idx].render_pid = game_mode_pid;
    }

    if (hint == MTKPOWER_HINT_VIDEO_MODE) {
        LOG_D("ptScnList[%d].render_pid = %d", idx, video_mode_pid);
        ptScnList[idx].render_pid = video_mode_pid;
    }

    if (hint == MTKPOWER_HINT_CAMERA_MODE) {
        LOG_D("ptScnList[%d].render_pid = %d", idx, camera_mode_pid);
        ptScnList[idx].render_pid = camera_mode_pid;
    }

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
        if(cmdSetting(cmd, NULL, &ptScnList[idx], value, NULL) == 1) {
            LOG_V("force_update");
            force_update = 1;
        }
    }

    //SPF:fix tiktok invalid acquire for X6856OS151-2711 by sifeng.tian 20250224 start
    if (check_invalid_acq(pid, tid, hint, idx)) {
        LOG_I("perfLockAcq invalid handle:%d, size:%d, pid:%d", handle, size, pid);
        resetScenario(idx, 1);
        free(ptScnList[idx].lock_rsc_list);
        return 0;
    }
    //SPF:fix tiktok invalid acquire for X6856OS151-2711 by sifeng.tian 20250224 end

    if(log_hasI)
        LOG_I("idx:%d hdl:%d hint:%d pid:%d tid:%d duration:%d lock_user:%s => ret_hdl:%d", idx, handle, hint, pid, (tid == 0) ? pid : tid, duration, ptScnList[idx].comm, ret_handle);
	else
        LOG_D("idx:%d hdl:%d hint:%d pid:%d duration:%d lock_user:%s => ret_hdl:%d", idx, handle, hint, pid, duration, ptScnList[idx].comm, ret_handle);

    if(hdl_enabled == 1) {
        if(rsc_modified == 1 || force_update == 1) {
            LOG_D("perflockAcq force update!!!");
            perfScnUpdate(idx, 1); // update setting
        }
    }
    else {
        perfScnEnable(idx);
    }

    free(ptScnList[idx].lock_rsc_list);

    return ret_handle;
}

int perfLockRel(int handle)
{
    int idx = -1;

    LOG_V("handle:%d", handle);

    Mutex::Autolock lock(sMutex);
    if (!nIsReady)
        if(!init()) return 0;

    if (!nPowerLibEnable)
        return 0;

    idx = findHandleToIndex(handle);
    LOG_I("hdl:%d, idx:%d", handle, idx);
    if(idx < nUserScnBase || idx >= nUserScnBase + REG_SCN_MAX)
        return -1;

    perfScnDisable(idx);

    //ptScnList[idx].lock_rsc_size = 0;

    if(ptScnList[idx].scn_state == STATE_ON)
        return -1;

    resetScenario(idx, 1);

    return 0;
}

int perfGetHintRsc(int hint, int *size, int *rsc_list)
{
    Mutex::Autolock lock(sMutex);
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
    Mutex::Autolock lock(sMutex);

    if (!nIsReady)
        if(!init()) return -1;

    return 0;
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
        RscCfgTbl[i].gameVal = RSC_TBL_INVALID_VALUE;
        if (RscCfgTbl[i].comp == SMALLEST)
            gRscCtlTbl[i].resetVal = RscCfgTbl[i].maxVal + 1;
        else if (RscCfgTbl[i].comp == BIGGEST)
            gRscCtlTbl[i].resetVal = RscCfgTbl[i].minVal - 1;
        else
            gRscCtlTbl[i].resetVal = RSC_TBL_INVALID_VALUE;

        gRscCtlTbl[i].curVal = gRscCtlTbl[i].resetVal;
        gRscCtlTbl[i].isValid = 1;
        gRscCtlTbl[i].log = 1;

        LOG_D("gRscCtlTbl[%d] resetVal:%d curVal:%d isValid:%d log:%d",
        i, gRscCtlTbl[i].resetVal, gRscCtlTbl[i].curVal, gRscCtlTbl[i].isValid, gRscCtlTbl[i].log);
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

int check_core_ctl_ioctl()
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

int load_fbc_api(void)
{
    void *handle = NULL, *func = NULL;

    handle = dlopen(FBC_LIB_FULL_NAME, RTLD_NOW);
    if (handle == NULL) {
        LOG_E("dlopen error: %s", dlerror());
        return -1;
    }

    func = dlsym(handle, "xgfGetFstbActive");
    xgfGetFstbActive = reinterpret_cast<fbc_get_fstb_active>(func);

    if (xgfGetFstbActive == NULL) {
        LOG_E("xgfGetFstbActive error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    func = dlsym(handle, "xgfWaitFstbActive");
    xgfWaitFstbActive = reinterpret_cast<fbc_wait_fstb_active>(func);

    if (xgfWaitFstbActive == NULL) {
        LOG_E("xgfWaitFstbActive error: %s", dlerror());
        dlclose(handle);
        return -1;
    }

    LOG_I("load fbc successfully");

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
    LOG_D("%d", value);

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

            notifyToCloseDB = reinterpret_cast<NotifyToCloseDBHook>(dlsym(libMBrainHandle, "NotifyToCloseDBHook"));
            if(nullptr == notifyToCloseDB) {
                LOG_E("notifyToCloseDB error: %s", dlerror());
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

//SPD:thermal ux modify for k510 by sifengtian 20230721 start
int getPpmSupport()
{
    return gtDrvInfo.ppmSupport;
}
//SPD:thermal ux modify for k510 by sifengtian 20230721 start


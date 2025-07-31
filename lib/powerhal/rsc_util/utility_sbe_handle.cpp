#include <vector>
#define LOG_TAG "libPowerHal"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)
#define LOG_NDEBUG 0

#include <string.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <unordered_map>
#include <utils/Trace.h>
#include "utility_sbe_handle.h"
#include "perfservice_types.h"
#include "perfservice.h"
#include "mtkperf_resource.h"
#include "mtkpower_hint.h"
#include "perfservice_xmlparse.h"
#include "perfservice_scn.h"

#define UX_SBE_NOT_SUPPORT          "SBE_Not_Support"
#define UX_SBE_SCROLL_SAVE_POWER    "UxScroll"
#define UX_SBE_SCROLL_ENABLE        1
#define UX_SBE_SCROLL_DISABLE       0
#define PACKAGE_NAME_LEN_MAX        256
#define PACKAGE_CODE_LEN_MAX        10
#define PACKAGE_DATA_LEN_MAX        10
#define SMART_LAUNCH_ANIMATION_TARGET_TIME 500
#define SMART_LAUNCH_BIG_CLUSTER_LOW_FREQ_LIMIT 1200000
#define SMART_LAUNCH_BUFFER_TIME_MS       50
#define SMART_LAUNCH_CAPABILITY_RATION    25
#define SMART_LAUNCH_CAPABILITY_SPAN_MAX  6


#define SMART_LAUNCH_PACKAGE_INFO_DATA_PATH       "/data/vendor/powerhal/powerdlttable.data"
#define PATH_PERF_IOCTL    "/proc/perfmgr/perf_ioctl"

#define ATRACE_MESSAGE_LEN 256
#define SMART_LAUNCH_HANDLE_TAG  "Smart_Launch_Handle"
#define SMART_LAUNCH_CALC_TAG    "Smart_Launch_Calc"
#define SMART_LAUNCH_CURRENT_FREQ_TAG "Smart_Launch_Curret_Freq"
#define SMART_LAUNCH_NEXT_OPP_TAG    "Smart_Launch_Next_OPP"

#define SETSYS_CODE_NOTIFY_SMART_LAUNCH_BOOT_START_END       0
#define SMART_LAUNCH_DATA_BOOT_START                         1
#define SMART_LAUNCH_DATA_BOOT_END                           0

#define SETSYS_CODE_NOTIFY_SMART_LAUNCH_BOOT_INFO     1
#define SMART_LAUNCH_TYPE_COLD_LAUNCH                 1
#define SMART_LAUNCH_TYPE_WARM_LAUNCH                 2
#define SMART_LAUNCH_TYPE_HOT_LAUNCH                  3

#define SETSYS_CODE_NOTIFY_SMART_LAUNCH_UNINSTALL_APP 2

#define PROP_SBE_CHECK_POINT_PROP             "vendor.boostfwk.rescue.checkpoint"
#define PROP_SBE_HORIZONTAL_SCROLL_DURATION   "vendor.boostfwk.scroll.duration.h"
#define PROP_SBE_VERTICAL_SCROLL_DURATION     "vendor.boostfwk.scroll.duration.v"
#define PROP_SBE_SBB_ENABLE_LATER             "vendor.boostfwk.sbb.touch.duration"
#define PROP_SBE_SCROLL_FREQ_FLOOR            "vendor.boostfwk.scroll.floor"
#define PROP_SBE_FRAME_DECISION               "vendor.boostfwk.frame.decision"

static Mutex sMutex;
static bool gLaunchPolicyInitFlag = false;
static int gClusterNumMax = -1;
static int gAnimationTimeTarget = SMART_LAUNCH_ANIMATION_TARGET_TIME;
static int gBigClusterDefaultFreq = -1;
static int gBigClusterOPPMinLevel = 15;
static int gBigClusterOPPCnt = 24;
static int gBigClusterDefaultOPP = 0;
static int gBigClusterCapabilityRation = 25;
static int gAdjustCapabilitySpanMax = SMART_LAUNCH_CAPABILITY_SPAN_MAX ;
static bool gSmartLaunchEnable = true;
static bool gIsOverPressureThreshold = false;
static bool gDefaultBigClusterFreqSupported = false;

int devfd = -1;

static struct system_pressure_info sysInfo;

extern std::unordered_map<std::string, int> gSmartLaunchPackageInfo;
extern tScnConTable tConTable[FIELD_SIZE];

typedef struct _SMART_LAUNCH_PACKAGE {
    int target_time;
    int feedback_time;
    int pre_opp;
    int next_opp;
    int capabilty_ration;
} SMART_LAUNCH_PACKAGE;


struct SBE_CurrentTaskInfo {
    char* sbeFeatureName;
    char* curentPackName;
    char* currentActName;
    bool  currentIsBoost;
    bool  whiteListTypeSupport;
    int   currentScnIndex;
};

typedef struct tHintMapTemp {
    int hintId;
    string hintCmd;
    int lock_rsc_size;
    int lock_rsc_list[MAX_ARGS_PER_REQUEST];
} tHintMapTemp;
tHintMapTemp gDefaultLuanchPolcy;

struct SBE_CurrentTaskInfo gSBE_UxScrollInfo;

void uxScrollSavePowerWithNoAppList(char* pack_name, char* act_name) {
    Mutex::Autolock lock(sMutex);

    gSBE_UxScrollInfo.curentPackName   = pack_name;
    gSBE_UxScrollInfo.currentActName   = act_name;
    gSBE_UxScrollInfo.currentScnIndex  = -1;
    gSBE_UxScrollInfo.sbeFeatureName   = (char*)UX_SBE_NOT_SUPPORT;
    gSBE_UxScrollInfo.whiteListTypeSupport = false;
    // ALOGI("pack:%s -> %s NOT_SUPPORT", pack_name, act_name);
    return;
}

int uxScrollSavePowerWithAppList(char* pack_name, char* act_name, char* sbe_featurename, int idex)
{
    Mutex::Autolock lock(sMutex);

    gSBE_UxScrollInfo.curentPackName  = pack_name;
    gSBE_UxScrollInfo.currentActName  = act_name;
    gSBE_UxScrollInfo.currentScnIndex = idex;

    //ALOGI("pack:%s->%s : SBE Feature name : %s", pack_name, act_name, sbe_featurename);

    if (sbe_featurename[0] == '\0') {
        gSBE_UxScrollInfo.sbeFeatureName   = (char*)UX_SBE_NOT_SUPPORT;
        gSBE_UxScrollInfo.whiteListTypeSupport = false;
        ALOGI("pack:%s->%s Not Has APPList", pack_name, act_name);
        return 1;
    }

    if (!strncmp(sbe_featurename, UX_SBE_SCROLL_SAVE_POWER, strlen(UX_SBE_SCROLL_SAVE_POWER))) {
        gSBE_UxScrollInfo.sbeFeatureName   = (char*)UX_SBE_SCROLL_SAVE_POWER;
        gSBE_UxScrollInfo.whiteListTypeSupport = true;
        gSBE_UxScrollInfo.currentIsBoost   = false;

        ALOGI("pack:%s->%s Has APPList: currentScnIndex = %d, Boost = %d", pack_name, act_name,
                                gSBE_UxScrollInfo.currentScnIndex, gSBE_UxScrollInfo.currentIsBoost);
        return 0;
    }
    return 1;
}

int uxScrollSavePower(int status, void *scn)
{
    Mutex::Autolock lock(sMutex);
    xml_activity* foreground_info = NULL;

    foreground_info = get_foreground_app_info();
    if (foreground_info == NULL) {
        ALOGE("get_foreground_app_info = NULL");
        return 0;
    }
/*
    ALOGI("status = %d, pack_name: %s, act_name: %s , forground pack: %s scnId = %d", status, gSBE_UxScrollInfo.curentPackName,
        gSBE_UxScrollInfo.currentActName, foreground_info->packName, gSBE_UxScrollInfo.currentScnIndex);
*/
    if (gSBE_UxScrollInfo.whiteListTypeSupport && (!strcmp(foreground_info->packName, gSBE_UxScrollInfo.curentPackName))) {
        if (gSBE_UxScrollInfo.currentScnIndex < 0)
                return -1;
       // ALOGI("Current Has WhiteListApp Support");

        if ((status == UX_SBE_SCROLL_ENABLE) && gSBE_UxScrollInfo.currentIsBoost) {
         //   ALOGI("Curren Task has Boost, Boost Failed");
            return 0;
        }

        if ((status == UX_SBE_SCROLL_ENABLE) && (!gSBE_UxScrollInfo.currentIsBoost)) {
            perfScnEnable(gSBE_UxScrollInfo.currentScnIndex);
            gSBE_UxScrollInfo.currentIsBoost = true;
          //  ALOGI("Curren Task Boost Success!!!");
            return 0;
        }

        if ((status == UX_SBE_SCROLL_DISABLE) && (!gSBE_UxScrollInfo.currentIsBoost)) {
          //  ALOGI("Curren Task has Disable, Disable Failed");
            return 0;
        }

        if ((status == UX_SBE_SCROLL_DISABLE) && gSBE_UxScrollInfo.currentIsBoost) {
            perfScnDisable(gSBE_UxScrollInfo.currentScnIndex);
            gSBE_UxScrollInfo.currentIsBoost = false;
          //  ALOGI("Curren Task Disable Success!!!");
            return 0;
        }
    }
    return 0;
}

int end_default_hold_time(int hindId, void* scn)
{
    if (hindId < 0)
        return -1;
    ALOGI("end_default_hold_time HindID : %d", hindId);
    if (hindId == MTKPOWER_HINT_PROCESS_CREATE ||
        hindId == MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE) {
        disableScenarioByHintId(MTKPOWER_HINT_PROCESS_CREATE);
        disableScenarioByHintId(MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE);
    }
    return 0;
}

int sbe_set_property_value(const char *propertyName, int val)
{
    char property_value[256];

    if (sprintf(property_value, "%d", val) < 0) {
        ALOGE("[sbe_set_property_value] sprintf %s failed!", propertyName);
        return -1;
    }

    property_set(propertyName, property_value);
    return 0;
}

int set_ux_sbe_check_point(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_CHECK_POINT_PROP, value);
    if (ret) {
        ALOGE("[set_ux_sbe_check_point] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_check_point(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_CHECK_POINT_PROP, 50);
    if (ret) {
        ALOGE("[init_ux_sbe_check_point] failed!");
        return -1;
    }
    return 0;
}

int set_ux_sbe_horizontal_duration(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_HORIZONTAL_SCROLL_DURATION, value);
    if (ret) {
        ALOGE("[set_ux_sbe_horizontal_duration] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_horizontal_duration(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_HORIZONTAL_SCROLL_DURATION, 700);
    if (ret) {
        ALOGE("[init_ux_sbe_horizontal_duration] failed!");
        return -1;
    }
    return 0;
}

int set_ux_sbe_vertical_duration(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_VERTICAL_SCROLL_DURATION, value);
    if (ret) {
        ALOGE("[set_ux_sbe_vertical_duration] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_vertical_duration(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_VERTICAL_SCROLL_DURATION, 3000);
    if (ret) {
        ALOGE("[init_ux_sbe_vertical_duration] failed!");
        return -1;
    }
    return 0;
}

int set_ux_sbe_sbb_enable_later(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_SBB_ENABLE_LATER, value);
    if (ret) {
        ALOGE("[set_ux_sbe_sbb_enable_later] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_sbb_enable_later(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_SBB_ENABLE_LATER, 1000);
    if (ret) {
        ALOGE("[init_ux_sbe_sbb_enable_later] failed!");
        return -1;
    }
    return 0;
}

int set_ux_sbe_enable_freq_floor(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_SCROLL_FREQ_FLOOR, value);
    if (ret) {
        ALOGE("[set_ux_sbe_enable_freq_floor] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_enable_freq_floor(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_SCROLL_FREQ_FLOOR, 0);
    if (ret) {
        ALOGE("[init_ux_sbe_enable_freq_floor] failed!");
        return -1;
    }
    return 0;
}

int set_ux_sbe_frame_decision(int value, void* scn)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_FRAME_DECISION, value);
    if (ret) {
        ALOGE("[set_ux_sbe_frame_decision] failed!");
        return -1;
    }
    return 0;
}

int init_ux_sbe_frame_decision(int value)
{
    int ret = 0;
    ret = sbe_set_property_value(PROP_SBE_FRAME_DECISION, 2);
    if (ret) {
        ALOGE("[init_ux_sbe_frame_decision] failed!");
        return -1;
    }
    return 0;
}

int parsePackageinfo(const char *packinfo, char* packname, int* code, int* data, int* launchtype)
{
    const char *packagePrefix = "Package:";
    const char *codePrefix = "Code:";
    const char *dataPrefix = "Data:";
    const char *typePrefix = "LaunchType:";
    const char *startPos, *endPos;
    int len = 0;
    char code_tbl[PACKAGE_CODE_LEN_MAX] = {'\0'};
    char data_tbl[PACKAGE_DATA_LEN_MAX] = {'\0'};
    char type_tbl[PACKAGE_DATA_LEN_MAX] = {'\0'};

    startPos = strstr(packinfo, packagePrefix);
    if (startPos) {
        startPos += strlen(packagePrefix);
        endPos = strchr(startPos, ',');
        len = endPos - startPos;
        if (len >= PACKAGE_NAME_LEN_MAX)
            len = PACKAGE_NAME_LEN_MAX - 1;

        if (endPos) {
            strncpy(packname, startPos, len);
            packname[len] = '\0';
        }
    }

    len = 0;
    startPos = strstr(packinfo, codePrefix);
    if (startPos) {
        startPos += strlen(codePrefix);
        endPos = strchr(startPos, ',');
        len = endPos - startPos;
        if (len >= PACKAGE_CODE_LEN_MAX)
            len = PACKAGE_CODE_LEN_MAX - 1;
        if (endPos) {
            strncpy(code_tbl, startPos, len);
            code_tbl[len] = '\0';
        }
    }

    len = 0;
    startPos = strstr(packinfo, dataPrefix);
    if (startPos) {
        startPos += strlen(dataPrefix);
        endPos = strchr(startPos, ',');
        len = endPos - startPos;
        if (len >= PACKAGE_DATA_LEN_MAX)
            len = PACKAGE_DATA_LEN_MAX - 1;
        if (endPos) {
            strncpy(data_tbl, startPos, len);
            data_tbl[len] = '\0';
        }
    }

    len = 0;
    startPos = strstr(packinfo, typePrefix);
    if (startPos) {
        startPos += strlen(typePrefix);
        endPos = strchr(startPos, ',');
        len = endPos - startPos;
        if (len >= PACKAGE_DATA_LEN_MAX)
            len = PACKAGE_DATA_LEN_MAX - 1;
        if (endPos) {
            strncpy(type_tbl, startPos, len);
            type_tbl[len] = '\0';
        }
    }

    *code = atoi(code_tbl);
    *data = atoi(data_tbl);
    *launchtype  = atoi(type_tbl);

    return 0;
}

bool getDefaultBigClusterFreqSupported()
{
    int i = 0;

    for (i = 0; i < gDefaultLuanchPolcy.lock_rsc_size; i++) {
        if (gDefaultLuanchPolcy.lock_rsc_list[i] == PERF_RES_CPUFREQ_MAX_CLUSTER_2) {
            return true;
        }
    }

    return false;
}

int getDefaultBigClusterFreq()
{
    int i = 0;
    int freq = -1;

    for (i = 0; i < gDefaultLuanchPolcy.lock_rsc_size; i++) {
        if (gDefaultLuanchPolcy.lock_rsc_list[i] == PERF_RES_CPUFREQ_MAX_CLUSTER_2) {
            freq = gDefaultLuanchPolcy.lock_rsc_list[i+1];
            return freq;
        }
    }

    return freq;
}

int getDefaultScnHintRsc(int hint, tHintMapTemp* hintres)
{

    hintres->lock_rsc_size = getHintRscSize(hint);
    if (hintres->lock_rsc_size <= 0 || hintres->lock_rsc_size> MAX_ARGS_PER_REQUEST)
        return -1;

    for(int i = 0; i < hintres->lock_rsc_size; i = i+2) {
        ALOGD("cmd: %x, param: %d", getHintRscElement(hint, i), getHintRscElement(hint, i+1));
    }

    if (getHintRscList(hint) != nullptr)
        memcpy(hintres->lock_rsc_list, getHintRscList(hint), sizeof(int)*(hintres->lock_rsc_size));
    else {
        ALOGE("[getHintRscList] hint_id %d cannot be found in PowerScnTbl", hint);
        return -1;
    }

    return 0;
}

int getDefaultTargetTime()
{
    int i;
    int time = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_TIME_TARGET")) {
            time = tConTable[i].defaultVal;
            return time >= 0 ? time : SMART_LAUNCH_ANIMATION_TARGET_TIME;
        }
    }

    return SMART_LAUNCH_ANIMATION_TARGET_TIME;
}

int getDefaultDlCPUFreqMax()
{
    int i;
    int freq = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_CLUSTER_2_CPU_MAX_FREQ")) {
            freq = tConTable[i].defaultVal;
            return freq > 0 ? freq : SMART_LAUNCH_BIG_CLUSTER_LOW_FREQ_LIMIT;
        }
    }

    return SMART_LAUNCH_BIG_CLUSTER_LOW_FREQ_LIMIT;
}

int getDefaultDlPowerMigration()
{
    int i;
    int value = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_CLUSTER_2_CAP_RATION")) {
            value = tConTable[i].defaultVal;
            return value > 0 ? value : SMART_LAUNCH_CAPABILITY_RATION;
        }
    }

    return SMART_LAUNCH_CAPABILITY_RATION;
}

int getDefaultOppOffsetScale()
{
    int i;
    int offset = -1;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_CLUSTER_2_OPP_SPA_MAX")) {
            offset = tConTable[i].defaultVal;
            return offset > 0 ? offset : SMART_LAUNCH_CAPABILITY_SPAN_MAX;
        }
    }

    return SMART_LAUNCH_CAPABILITY_SPAN_MAX;
}

int getDefaultMemPSIThreshold()
{
    int i;
    int threshold = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_MEM_PSI_THRESHOLD")) {
            threshold = tConTable[i].defaultVal;
            return threshold > 0 ? threshold : MEM_THRESHOLD_DEFAULT;
        }
    }

    return MEM_THRESHOLD_DEFAULT;
}

int getDefaultCpuPSIThreshold()
{
    int i;
    int threshold = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_CPU_PSI_THRESHOLD")) {
            threshold = tConTable[i].defaultVal;
            return threshold > 0 ? threshold : CPU_THRESHOLD_DEFAULT;
        }
    }

    return CPU_THRESHOLD_DEFAULT;
}

int getDefaultIoPSIThreshold()
{
    int i;
    int threshold = 0;
    for (i = 0; i < FIELD_SIZE; i++) {
        if (!strcmp(tConTable[i].cmdName.c_str(), "PERF_RES_SMART_LAUNCH_IO_PSI_THRESHOLD")) {
            threshold = tConTable[i].defaultVal;
            return threshold > 0 ? threshold : IO_THRESHOLD_DEFAULT;
        }
    }

    return IO_THRESHOLD_DEFAULT;
}

void tracePrintSmartLaunchDataInfo(const char* tag, int data)
{
    if(ATRACE_ENABLED())
        ATRACE_INT(tag, data);
}


int resetLaunchPolicyToDefault()
{
    if (gDefaultBigClusterFreqSupported) {
        HintRscList_modify(MTKPOWER_HINT_PROCESS_CREATE, PERF_RES_CPUFREQ_MAX_CLUSTER_2, gBigClusterDefaultFreq);
        tracePrintSmartLaunchDataInfo(SMART_LAUNCH_CURRENT_FREQ_TAG, gBigClusterDefaultFreq);
        ALOGD("resetLaunchPolicyToDefault %d, core num = %d, opp = %d", gBigClusterDefaultFreq, gClusterNumMax, gBigClusterDefaultOPP);
    } else {
        HintRscList_remove(MTKPOWER_HINT_PROCESS_CREATE, PERF_RES_CPUFREQ_MAX_CLUSTER_2);
        tracePrintSmartLaunchDataInfo(SMART_LAUNCH_CURRENT_FREQ_TAG, 0);
    }

    return 0;
}
void initSystemPressureStats()
{
    int i = 0;

    sysInfo.mem_data.filename = PRESSURE_PATH_MEMORY;
    sysInfo.cpu_data.filename = PRESSURE_PATH_CPU;
    sysInfo.io_data.filename  = PRESSURE_PATH_IO;

    sysInfo.mem_threshold = getDefaultMemPSIThreshold();
    sysInfo.cpu_threshold = getDefaultCpuPSIThreshold();
    sysInfo.io_threshold  = getDefaultIoPSIThreshold();

    for (i = 0; i < PSI_TYPE_COUNT; i++) {
        sysInfo.mem_data.stats[i].avg10  = 0.0;
        sysInfo.mem_data.stats[i].avg60  = 0.0;
        sysInfo.mem_data.stats[i].avg300 = 0.0;
        sysInfo.mem_data.stats[i].total  = 0;
        sysInfo.cpu_data.stats[i].avg10  = 0.0;
        sysInfo.cpu_data.stats[i].avg60  = 0.0;
        sysInfo.cpu_data.stats[i].avg300 = 0.0;
        sysInfo.cpu_data.stats[i].total  = 0;
        sysInfo.io_data.stats[i].avg10   = 0.0;
        sysInfo.io_data.stats[i].avg60   = 0.0;
        sysInfo.io_data.stats[i].avg300  = 0.0;
        sysInfo.io_data.stats[i].total   = 0;
    }
}

int parseSystemPressureInfo(const char* file_path, struct pressure_stats* stats)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        ALOGE("Error opening file: %s", file_path);
        return -1;
    }

    if (fscanf(file, "some avg10=%lf avg60=%lf avg300=%lf total=%llu\n",
        &stats[PSI_TYPE_SOME].avg10, &stats[PSI_TYPE_SOME].avg60, &stats[PSI_TYPE_SOME].avg300,
        &stats[PSI_TYPE_SOME].total) != 4) {
            ALOGE("Error parsing 'some' pressure data from %s.", file_path);
            if (0 != fclose(file)) {
              ALOGE("Error close file: %s", file_path);
              return -1;
            }
            return -1;
    }

    if (fscanf(file, "full avg10=%lf avg60=%lf avg300=%lf total=%llu\n",
        &stats[PSI_TYPE_FULL].avg10, &stats[PSI_TYPE_FULL].avg60, &stats[PSI_TYPE_FULL].avg300,
        &stats[PSI_TYPE_FULL].total) != 4) {
            ALOGE("Error parsing 'full' pressure data from %s.", file_path);
            if (0 != fclose(file)) {
              ALOGE("Error close file: %s", file_path);
              return -1;
            }
            return -1;
    }

    if (0 != fclose(file)) {
        ALOGE("Error close file: %s", file_path);
        return -1;
    }
    return 0;
}

int getSystemPressureInfo()
{
    int ret = -1;

    ret = parseSystemPressureInfo(sysInfo.mem_data.filename, sysInfo.mem_data.stats);
    if (ret < 0) {
        ALOGE("Error get 'mem' pressure data from %s.", sysInfo.mem_data.filename);
        return ret;
    }

    ret = parseSystemPressureInfo(sysInfo.cpu_data.filename, sysInfo.cpu_data.stats);
    if (ret < 0) {
        ALOGE("Error get 'cpu' pressure data from %s.", sysInfo.mem_data.filename);
        return ret;
    }

    ret = parseSystemPressureInfo(sysInfo.io_data.filename, sysInfo.io_data.stats);
    if (ret < 0) {
        ALOGE("Error get 'io' pressure data from %s.", sysInfo.mem_data.filename);
        return ret;
    }

    for (int i = 0; i < PSI_TYPE_COUNT; i++) {
        ALOGD("mem avg10 %lf"  ,sysInfo.mem_data.stats[i].avg10);
        ALOGD("mem avg60 %lf"  ,sysInfo.mem_data.stats[i].avg60);
        ALOGD("mem avg300 %lf" ,sysInfo.mem_data.stats[i].avg300);
        ALOGD("mem total %llu" ,sysInfo.mem_data.stats[i].total);

        ALOGD("cpu avg10 %lf" ,sysInfo.cpu_data.stats[i].avg10);
        ALOGD("cpu avg60 %lf" ,sysInfo.cpu_data.stats[i].avg60);
        ALOGD("cpu avg300 %lf" ,sysInfo.cpu_data.stats[i].avg300);
        ALOGD("cpu total %llu", sysInfo.cpu_data.stats[i].total);

        ALOGD("io avg10 %lf", sysInfo.io_data.stats[i].avg10);
        ALOGD("io avg60 %lf", sysInfo.io_data.stats[i].avg60);
        ALOGD("io avg300 %lf", sysInfo.io_data.stats[i].avg300);
        ALOGD("io total %llu", sysInfo.io_data.stats[i].total);
    }

    return 0;
}
bool isSystemOverPressureThreshold(float memThreshold, float cpuThreshold, float ioThreshold)
{
    int ret = -1;
    float memAvg = 0.0;
    float cpuAvg = 0.0;
    float ioAvg  = 0.0;

    gIsOverPressureThreshold = false;
    ret = getSystemPressureInfo();
    if (ret < 0) {
        gIsOverPressureThreshold = true;
        ALOGE("Error: get system pressure info");
        return true;
    }

    memAvg = sysInfo.mem_data.stats[PSI_TYPE_FULL].avg10;
    cpuAvg = sysInfo.cpu_data.stats[PSI_TYPE_SOME].avg10;
    ioAvg  = sysInfo.io_data.stats[PSI_TYPE_FULL].avg10;

    if (memAvg >= memThreshold || cpuAvg >= cpuThreshold || ioAvg >= ioThreshold) {
        ALOGE("memAvg = %f, cpuAvg = %f, ioAvg = %f", memAvg, cpuAvg, ioAvg);
        gIsOverPressureThreshold = true;
        return true;
    }

    return false;
}

int pickCurrentFreq(const char *packname)
{
    int bigcluster_opp = 0;
    int current_freq = gBigClusterDefaultFreq;
    ALOGD("pickCurrentFreq: %s", packname);

    if (!gAnimationTimeTarget) {
        return gBigClusterDefaultFreq;
    }

    auto search = gSmartLaunchPackageInfo.find(packname);
    if (search != gSmartLaunchPackageInfo.end()) {
        if (isSystemOverPressureThreshold(sysInfo.mem_threshold, sysInfo.cpu_threshold, sysInfo.io_threshold)) {
            ALOGE("isSystemOverPressureThreshold true");
            return gBigClusterDefaultFreq;
        }
        bigcluster_opp = gSmartLaunchPackageInfo[packname];
        current_freq =  getCPUFreq(gClusterNumMax - 1, bigcluster_opp);
    } else {
        current_freq = gBigClusterDefaultFreq;
    }

    return current_freq;
}

void handleCurrentLaunchPolicy(const char *packname)
{
    int freq;
    freq = pickCurrentFreq(packname);
    if (gDefaultBigClusterFreqSupported) {
        HintRscList_modify(MTKPOWER_HINT_PROCESS_CREATE, PERF_RES_CPUFREQ_MAX_CLUSTER_2, freq);
    } else {
        if (freq != gBigClusterDefaultFreq) {
            HintRscList_append(MTKPOWER_HINT_PROCESS_CREATE, PERF_RES_CPUFREQ_MAX_CLUSTER_2, freq);
        } else {
            HintRscList_remove(MTKPOWER_HINT_PROCESS_CREATE, PERF_RES_CPUFREQ_MAX_CLUSTER_2);
        }
    }

    tracePrintSmartLaunchDataInfo(SMART_LAUNCH_CURRENT_FREQ_TAG, freq);
    return;
}

int modifyNextFreqAndSaveToDisk(const char *packname, int next_opp)
{
    int ret = 0;
    gSmartLaunchPackageInfo[packname] = next_opp;
    string str = to_string(next_opp);
    ret = modifyElement2DlTable(SMART_LAUNCH_PACKAGE_INFO_DATA_PATH, packname, str.c_str());
    if (ret < 0) {

    }
    return next_opp;
}

int insertNewFreqAndInsertToDisk(const char *packname, int next_opp)
{
    int ret = 0;
    gSmartLaunchPackageInfo[packname] = next_opp;
    string str = to_string(next_opp);
    ret = insertNewElement2DlTable(SMART_LAUNCH_PACKAGE_INFO_DATA_PATH, packname, str.c_str());
    if (ret < 0) {

    }
    return 0;
}

void deletePackageAndUpdateToDisk(const char *packname)
{
    int ret = 0;
    gSmartLaunchPackageInfo.erase(packname);
    ret = deleteElement2DlTable(SMART_LAUNCH_PACKAGE_INFO_DATA_PATH, packname);
    if (ret < 0) {

    }
}

int clampFreqOpp(int next_opp)
{
    if (next_opp <= gBigClusterDefaultOPP)
        next_opp = gBigClusterDefaultOPP;
    if (next_opp >= gBigClusterOPPMinLevel)
        next_opp = gBigClusterOPPMinLevel;
    return next_opp;
}

int uclampRation(int migration)
{
    if (migration <= 0)
        migration = 0;
    if (migration >= 100)
        migration = 100;
    return migration;
}

static int check_perf_ioctl_valid(void)
{
    if (devfd >= 0) {
        return 0;
    } else if (devfd < 0) {
        devfd = open(PATH_PERF_IOCTL, O_RDONLY);
        if (devfd < 0) {
            ALOGE("Can't open %s", PATH_PERF_IOCTL);
            return -1;
        }
    }
    return 0;
}

int coreAlgorith(int feedback_time, int target_time, int pre_opp, int ration)
{
    int next_opp = pre_opp;
    SMART_LAUNCH_PACKAGE msg;
    int ret = 0;

    msg.feedback_time = feedback_time;
    msg.target_time   = target_time;
    msg.pre_opp       = pre_opp;
    msg.capabilty_ration = ration;
    msg.next_opp      = -1;

    if (check_perf_ioctl_valid() == 0)
        ioctl(devfd, SMART_LAUNCH_ALGORITHM, &msg);

    next_opp = msg.next_opp;
    ALOGD("coreAlgorith: next_opp = %d", next_opp);

    return next_opp >= 0 ? clampFreqOpp(next_opp) : pre_opp;
}

void storeDefaultLaunchBoostPolicy()
{
    int ret = 0;

    initSystemPressureStats();

    gDefaultLuanchPolcy.hintId   = MTKPOWER_HINT_PROCESS_CREATE;
    gDefaultLuanchPolcy.hintCmd  = "MTKPOWER_HINT_PROCESS_CREATE";
    gDefaultLuanchPolcy.lock_rsc_size = -1;

    ret = getDefaultScnHintRsc(MTKPOWER_HINT_PROCESS_CREATE, &gDefaultLuanchPolcy);
    if (ret < 0) {
        gSmartLaunchEnable = false;
        ALOGE("gDefaultLuanchPolcy size: %d", gDefaultLuanchPolcy.lock_rsc_size);
        return;
    }

    for (int i = 0; i < gDefaultLuanchPolcy.lock_rsc_size; i++)
        ALOGD("gDefaultLuanchPolcy: %d", gDefaultLuanchPolcy.lock_rsc_list[i]);

    gAnimationTimeTarget = getDefaultTargetTime();
    gClusterNumMax = getClusterNum();
    gDefaultBigClusterFreqSupported = getDefaultBigClusterFreqSupported();
    if (gDefaultBigClusterFreqSupported) {
        gBigClusterDefaultFreq = getDefaultBigClusterFreq();
        gBigClusterOPPCnt = getCPUFreqOppCnt(gClusterNumMax -1);
        gBigClusterDefaultOPP = getCPUFreqToOpp(gClusterNumMax -1, gBigClusterDefaultFreq);
        gBigClusterDefaultFreq = getCPUFreq(gClusterNumMax -1, gBigClusterDefaultOPP);
    } else {
        gBigClusterDefaultFreq = getCPUFreq(gClusterNumMax -1, 0);
    }
    gBigClusterOPPMinLevel = getCPUFreqToOpp(gClusterNumMax -1, getDefaultDlCPUFreqMax());
    gBigClusterCapabilityRation = uclampRation(getDefaultDlPowerMigration());
    gAdjustCapabilitySpanMax = getDefaultOppOffsetScale();


    gLaunchPolicyInitFlag = true;
    gSmartLaunchEnable= true;

    return;
}

int handlePackageUninstall(const char *packname)
{
    auto search = gSmartLaunchPackageInfo.find(packname);
    if (search != gSmartLaunchPackageInfo.end())
        deletePackageAndUpdateToDisk(packname);
    return 0;
}

bool checkIfNeedAdjust(int feedback_time)
{
    int delta_time = abs(feedback_time - gAnimationTimeTarget);
    if (delta_time <= (SMART_LAUNCH_BUFFER_TIME_MS/2))
        return false;

    return true;
}

void calcNextLaunchPower(const char *packname, int feedback_time)
{
    int next_opp = 0;
    int delta_opp = 0;

    if (!gAnimationTimeTarget)
        return;

    if (gIsOverPressureThreshold)
        return;

    if (!checkIfNeedAdjust(feedback_time))
        return;

    auto search = gSmartLaunchPackageInfo.find(packname);
    if (search != gSmartLaunchPackageInfo.end()) {
        int pre_opp = gSmartLaunchPackageInfo[packname];
        next_opp = coreAlgorith(feedback_time, gAnimationTimeTarget, pre_opp, gBigClusterCapabilityRation);
        if (next_opp <= gBigClusterDefaultOPP) {
            ALOGD("deletePackageAndUpdateToDisk: next_opp = %d", next_opp);
            deletePackageAndUpdateToDisk(packname);
            tracePrintSmartLaunchDataInfo(SMART_LAUNCH_NEXT_OPP_TAG, next_opp);
            return;
        } else {
            delta_opp = next_opp - pre_opp;
            if (delta_opp >= SMART_LAUNCH_CAPABILITY_SPAN_MAX) {
                next_opp = pre_opp + SMART_LAUNCH_CAPABILITY_SPAN_MAX;
            }
            if (next_opp != pre_opp) {
                ALOGD("modifyNextFreqAndSaveToDisk: next_opp = %d", next_opp);
                modifyNextFreqAndSaveToDisk(packname, next_opp);
                tracePrintSmartLaunchDataInfo(SMART_LAUNCH_NEXT_OPP_TAG, next_opp);
                return;
            }
        }
    } else {
        next_opp = coreAlgorith(feedback_time, gAnimationTimeTarget, gBigClusterDefaultOPP, gBigClusterCapabilityRation);
        if (next_opp > gBigClusterDefaultOPP) {
            delta_opp = next_opp - gBigClusterDefaultOPP;
            if (delta_opp >= SMART_LAUNCH_CAPABILITY_SPAN_MAX) {
                next_opp = gBigClusterDefaultOPP + SMART_LAUNCH_CAPABILITY_SPAN_MAX;
            }
            ALOGD("insertNewFreqAndInsertToDisk next_opp = %d", next_opp);
            insertNewFreqAndInsertToDisk(packname, next_opp);
            tracePrintSmartLaunchDataInfo(SMART_LAUNCH_NEXT_OPP_TAG, next_opp);
            return;
        }
    }

    return;
}

int notifySmartLaunchEngine(const char *packinfo)
{
    char packname[PACKAGE_NAME_LEN_MAX] = {'\0'};
    int code = -1;
    int launchtype = -1;
    int data = -1;

    if (!gLaunchPolicyInitFlag)
        storeDefaultLaunchBoostPolicy();

    if ((!packinfo) || (gSmartLaunchEnable == false))
        return 0;

    parsePackageinfo(packinfo, packname, &code, &data, &launchtype);

    switch(code) {
        case SETSYS_CODE_NOTIFY_SMART_LAUNCH_BOOT_START_END:
            if (data == SMART_LAUNCH_DATA_BOOT_START) {
                handleCurrentLaunchPolicy(packname);
            } else {
                resetLaunchPolicyToDefault();
            }
        break;

        case SETSYS_CODE_NOTIFY_SMART_LAUNCH_BOOT_INFO:
            if (launchtype == SMART_LAUNCH_TYPE_COLD_LAUNCH) {
                calcNextLaunchPower(packname, data);
                resetLaunchPolicyToDefault();
            }
        break;

        case SETSYS_CODE_NOTIFY_SMART_LAUNCH_UNINSTALL_APP:
            handlePackageUninstall(packname);
        break;

        default:
            ALOGI("unknown type");
        break;
    }

    return 0;
}

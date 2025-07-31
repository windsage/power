#define LOG_TAG "SbeHandle"

#define LOG_NDEBUG 0

#include <string.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include "utility_sbe_handle.h"
#include "perfservice_types.h"
#include "perfservice.h"
//SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 start 
#include "mtkpower_hint.h"
//SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 end

#define UX_SBE_NOT_SUPPORT          "SBE_Not_Support"
#define UX_SBE_SCROLL_SAVE_POWER    "UxScroll"
#define UX_SBE_SCROLL_ENABLE        1
#define UX_SBE_SCROLL_DISABLE       0

#define PROP_SBE_CHECK_POINT_PROP             "vendor.boostfwk.rescue.checkpoint"
#define PROP_SBE_HORIZONTAL_SCROLL_DURATION   "vendor.boostfwk.scroll.duration.h"
#define PROP_SBE_VERTICAL_SCROLL_DURATION     "vendor.boostfwk.scroll.duration.v"
#define PROP_SBE_SBB_ENABLE_LATER             "vendor.boostfwk.sbb.touch.duration"
#define PROP_SBE_SCROLL_FREQ_FLOOR            "vendor.boostfwk.scroll.floor"
#define PROP_SBE_FRAME_DECISION               "vendor.boostfwk.frame.decision"

static Mutex sMutex;

struct SBE_CurrentTaskInfo {
    char* sbeFeatureName;
    char* curentPackName;
    char* currentActName;
    bool  currentIsBoost;
    bool  whiteListTypeSupport;
    int   currentScnIndex;
};

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
    //SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 start 
    // if (hindId < 0)
    //     return -1;
    // ALOGI("end_default_hold_time HindID : %d", hindId);
    // disableScenarioByHintId(hindId);
    // return 0;

    if (hindId < 0)
        return -1;
    ALOGI("end_default_hold_time HindID : %d", hindId);
    if (hindId == MTKPOWER_HINT_PROCESS_CREATE ||
        hindId == MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE) {
        disableScenarioByHintId(MTKPOWER_HINT_PROCESS_CREATE);
        disableScenarioByHintId(MTKPOWER_HINT_PROCESS_CREATE_PERF_MODE);
    }
    return 0;
    //SPF:fix issues for SBE_LAUNCH_END by sifeng.tian 20241206 end
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

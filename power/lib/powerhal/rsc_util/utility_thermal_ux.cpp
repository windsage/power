/**
  *Copyright © 2023 Transsion Inc
  *Author sifeng.tian
  *compute the frequency limit under high temperature
  */

#define LOG_TAG "Thermal_UX_POLICY"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include <utils/Log.h>
#include <utils/RefBase.h>
#include <cutils/properties.h>
#include <cutils/trace.h>
#include <dlfcn.h>
#include <hidl/HidlSupport.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <expat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include "perfservice.h"
#include "mtkpower_types.h"
#include "perfservice_types.h"
#include "mtkpower_hint.h"
#include "common.h"
#include "tran_common.h"
#include "utility_thermal_ux.h"

using std::string;
using std::vector;

#define THERMAL_UX_PROP              "ro.vendor.powerhal.thermal_ux_support"
#define PPM_THERMAL_UX               "/proc/ppm/policy/thermal_ux"
#define THERMAL_VALUE_MIN_PROP       "persist.vendor.powerhal.thermal_ux_temp_min"
#define THERMAL_VALUE_MAX_PROP       "persist.vendor.powerhal.thermal_ux_temp_max"
#define THERMAL_VALUE_MIN_DEF 42000
#define THERMAL_VALUE_MAX_DEF 52000


extern _cpufreq thermal_freq[CLUSTER_MAX];

bool thermal_ux_policy_enable = false;
static unsigned int mClusternum = 0;
static const char* mPlatform;
static unsigned long mCurrentTemp;
static char thermal_temp_patch[128];
static int mThermalTempMax;
static int mThermalTempMin;

static int check_thermalUx_ioctl_valid(void) {

    int fd = open(PPM_THERMAL_UX, O_WRONLY);
    if (fd == -1) {
        ALOGE("Could not open '%s'\n", PPM_THERMAL_UX);
        char *err_str = strerror(errno);
        ALOGE("error : %d, %s\n", errno, err_str);
        return -1;
    }
    close(fd);

    return 0;
}

static int dataInit(int clusterNum)
{
    if (clusterNum <= 0) {
        ALOGE("cluster info is invail");
        thermal_ux_policy_enable = false;
        return -1;
    }

    mClusternum = clusterNum;

    return 0;
}

static int propInit()
{
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    property_get("ro.board.platform", prop_content, "0");
    mPlatform = prop_content;

    if ((thermal_ux_policy_enable = get_property_value(THERMAL_UX_PROP)) == 0) {
        ALOGD("prop do not supoort thermal_ux");
        return -1;
    }

    mThermalTempMax = get_property_value(THERMAL_VALUE_MAX_PROP) > 0 ? get_property_value(THERMAL_VALUE_MAX_PROP) : THERMAL_VALUE_MAX_DEF;
    mThermalTempMin = get_property_value(THERMAL_VALUE_MIN_PROP) > 0 ? get_property_value(THERMAL_VALUE_MIN_PROP) : THERMAL_VALUE_MIN_DEF;

    return 0;
}


static int updateTemperature()
{
    if (strlen(thermal_temp_patch) > 0) {
        FILE* file = NULL;
        char line[256];
        int temp;

        file = fopen(thermal_temp_patch, "r");
        if (file) {
            char *str = NULL;
            if (fgets(line, sizeof(line), file)) {
                temp = atoi(line);
            }

            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
        }

        mCurrentTemp = temp;
    } else {
        struct dirent *ptr;
        DIR *dir;
        string PATH = "/sys/class/thermal/";
        dir = opendir(PATH.c_str());
        vector<string> filePaths;
        FILE* file = NULL;
        char line[256];

        while ((ptr=readdir(dir))!=NULL)
        {
            if (ptr->d_name[0] == '.')
                continue;

            filePaths.push_back(PATH + ptr->d_name);
        }
        closedir(dir);

        for (int i = 0; i < filePaths.size(); i++)
        {
            bool match = false;
            if (strstr(filePaths[i].c_str(), "cooling_device") != NULL) {
                continue;
            }

            file = fopen((filePaths[i] + "/type").c_str(), "r");
            if (file) {
                if (fgets(line, sizeof(line), file)) {
                    if (strncmp(line, "mtktsAP", 7) == 0 || strncmp(line, "ap_ntc", 6) == 0) {
                        strcpy(thermal_temp_patch, (filePaths[i] + "/temp").c_str());
                        match = true;
                    }
                }

                if(fclose(file) == EOF)
                    ALOGE("fclose errno:%d", errno);
                if (match)
                    break;
            }
        }
        if (file != NULL)
            fclose(file);

        ALOGD("thermal_temp_patch :%s", thermal_temp_patch);
        if (strlen(thermal_temp_patch) < 18) {
            ALOGE("can`t find ap thermal sys");
            thermal_ux_policy_enable = false;
            return -1;
        }
    }

    return 0;
}

bool is_valid_temp()
{
    updateTemperature();

    if (mThermalTempMin >= mThermalTempMax)
        return false;

    return (mCurrentTemp > mThermalTempMin && mCurrentTemp < mThermalTempMax) ? true : false;
}



void thermalUxPolicyInit(int clusterNum)
{
    if (propInit() == -1 || dataInit(clusterNum) == -1 || get_property_value("ro.vendor.mtk_thermal_2_0") == 1) {
        ALOGE("Thermal UX Policy Init Fail");
        return;
    }
}

//SPD: add powerhal reinit by sifengtian 20230711 start
void thermalUxPolicyReInit()
{
    ALOGI("[reinit] thermal ux: %d %d", mThermalTempMin, mThermalTempMax);
    mThermalTempMax = get_property_value(THERMAL_VALUE_MAX_PROP) > 0 ? get_property_value(THERMAL_VALUE_MAX_PROP) : THERMAL_VALUE_MAX_DEF;
    mThermalTempMin = get_property_value(THERMAL_VALUE_MIN_PROP) > 0 ? get_property_value(THERMAL_VALUE_MIN_PROP) : THERMAL_VALUE_MIN_DEF;
    ALOGI("[reinit] thermal ux: %d %d", mThermalTempMin, mThermalTempMax);
}
//SPD: add powerhal reinit by sifengtian 20230711 end

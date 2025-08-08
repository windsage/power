/* Copyright Statement:
  *
  *
  * Transsion Inc. (C) 2010. All rights reserved.
  *
  *
  * The following software/firmware and/or related documentation ("Transsion
  * Software") have been modified by Transsion Inc. All revisions are subject to
  * any receiver's applicable license agreements with Transsion Inc.
*/

#define LOG_TAG "libPowerHal"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include "perfservice.h"
#include "common.h"
#include "utility_cpuset.h"
#include "perfservice_prop.h"

//SPD: add for control dex2oat cpu by fan.feng1 20230619 start
#define PATH_DEV_CPUSET_DEX2OAT_CPUS "/dev/cpuset/dex2oat/cpus"
#define POWER_PROP_DEX2OAT_THREADS       "persist.odm.tr_powerhal.dex2oat_threads.model"

static int cpu_num = get_cpu_num();
static int cpu_max = (1 << cpu_num) - 1;
static int dex2oat_cgroup_enabled = 0;
static char dex2oat_default_cpus[32] = {0};
static char dex2oat_default_threads[PROPERTY_VALUE_MAX] = "\0";
static bool is_dex2oat_disable = false;

static void do_set_dex2oat_cpus(int value) {
    int i;
    char str[64] = {0};
    char final_str[128] = {0};
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int  prop_value = 0;
    int  thread_num = 0;

    if (strlen(dex2oat_default_cpus) == 0) {
        property_get(POWER_PROP_BOOT_COMPLETE, prop_content, "0");
        prop_value = atoi(prop_content);
        if (prop_value == 1) {
            get_str_value(PATH_DEV_CPUSET_DEX2OAT_CPUS, dex2oat_default_cpus, sizeof(dex2oat_default_cpus)-1);
            property_get(POWER_PROP_DEX2OAT_THREADS, dex2oat_default_threads, "4");
            LOG_I("dex2oat cpuset bitmask: 0x%x,cpus = %s", value, dex2oat_default_cpus);
        } else {
            return;
        }
    }

    if (value <= 0 || value > cpu_max) {
        LOG_E("invalid input:%d (max:%d)", value, cpu_max);
        return;
    }

    for (i = 0; i < cpu_num; i++) {
        if ((value & (1 << i)) > 0) {
            if(snprintf(str, 64, "%d-%d,", i, i) < 0) {
                LOG_E("snprintf error");
                return;
            }
            strncat(final_str, str, strlen(str));
            thread_num++;
        }
    }

    memset(prop_content,0,PROPERTY_VALUE_MAX);
    sprintf(prop_content, "%d", thread_num);

    set_value(PATH_DEV_CPUSET_DEX2OAT_CPUS, final_str);
    property_set(POWER_PROP_DEX2OAT_THREADS, prop_content);
}

int init_dex2oat_cpus(int power_on)
{
    struct stat stat_buf;

    LOG_V("%d", power_on);
    dex2oat_cgroup_enabled = (0 == stat(PATH_DEV_CPUSET_DEX2OAT_CPUS, &stat_buf)) ? 1 : 0;

    return 0;
}

int set_cpuset_dex2oat_cpus(int value, void *scn)
{
    if(!dex2oat_cgroup_enabled || is_dex2oat_disable)
        return -1;

    do_set_dex2oat_cpus(value);

    return 0;
}

int unset_cpuset_dex2oat_cpus(int value, void *scn)
{
    LOG_I("is unset_cpuset_dex2oat_cpus");

    if(!dex2oat_cgroup_enabled || is_dex2oat_disable)
        return -1;

    if (strlen(dex2oat_default_cpus) == 0)
        return -1;

    set_value(PATH_DEV_CPUSET_DEX2OAT_CPUS, dex2oat_default_cpus);
    property_set(POWER_PROP_DEX2OAT_THREADS, dex2oat_default_threads);

    return 0;
}

int set_thermal_dex2oat_cpus(int value,void *scn)
{
    LOG_I("is thermal_dex2oat_cpus");

    if(!dex2oat_cgroup_enabled)
        return -1;

    is_dex2oat_disable = true;

    do_set_dex2oat_cpus(value);

    return 0;
}

int unset_thermal_dex2oat_cpus(int value,void *scn)
{
    LOG_I("is unset_thermal_dex2oat_cpus");

    if(!dex2oat_cgroup_enabled)
        return -1;

    if (strlen(dex2oat_default_cpus) == 0)
        return -1;

    is_dex2oat_disable = false;

    set_value(PATH_DEV_CPUSET_DEX2OAT_CPUS, dex2oat_default_cpus);
    property_set(POWER_PROP_DEX2OAT_THREADS, dex2oat_default_threads);

    return 0;
}
//SPD: add for control dex2oat cpu by fan.feng1 20230619 end


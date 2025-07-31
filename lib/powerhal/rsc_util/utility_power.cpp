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
#include <dlfcn.h>

#include "common.h"
#include "utility_power.h"

#define PERF_MODE "performance"
#define BALANCE_MODE "balance"

int default_core_max[TRAN_MAX_MODE][3];

void init_power_mode(void)
{
    char line[256];
    FILE* file = NULL;
    struct stat stat_buf;
    time_t data_mtime = 0, system_mtime = 0;
    int index = 0;

    memset(default_core_max, 0, sizeof(default_core_max));

    if (0 == stat(PATH_POWER_MODE_FILE_2, &stat_buf))
        data_mtime = stat_buf.st_mtime;

    if (0 == stat(PATH_POWER_MODE_FILE, &stat_buf))
        system_mtime = stat_buf.st_mtime;

    if((data_mtime == 0) && (system_mtime ==0)) {
        ALOGE("not found power_mode file!");
        return;
    } else if (data_mtime < system_mtime) {
        file = fopen(PATH_POWER_MODE_FILE, "r");
    } else {
        file = fopen(PATH_POWER_MODE_FILE_2, "r");
    }

    if (file) {
            while (fgets(line, sizeof(line), file)) {
                if((!strstr(line, PERF_MODE)) && (!strstr(line, BALANCE_MODE))) {
                    continue;
                }

                if(index > TRAN_MAX_MODE) {
                    fclose(file);
                    return;
                }

                char *token = strtok(line, ",");
                int cpu_num = 0;
                while(token) {
                    token = strtok(NULL, "-");
                    if(token && cpu_num < 3) {
                        default_core_max[index][cpu_num++] = atoi(token);
                    }
                }
                index++;
            }
            if(fclose(file) == EOF)
                ALOGE("fclose errno:%d", errno);
    }
}

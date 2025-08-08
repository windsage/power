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

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>

#include <utils/Log.h>

#include "common.h"

 /*   return value:
  *         0, error or read nothing
  *        !0, read counts
  */
static
int read_from_file(const char* path, char* buf, int size)
{
    if (!path) {
        return 0;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        ALOGE("Could not open '%s'\n", path);
        char *err_str = strerror(errno);
        ALOGD("error : %d, %s\n", errno, err_str);
        return 0;
    }

    int count = read(fd, buf, size);
    if (count > 0) {
        count = (count < size) ? count : size - 1;
        while (count > 0 && buf[count-1] == '\n') count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(fd);
    return count;
}

static
int write_to_file(const char* path, const char* buf, int size)
{
    if (!path) {
        ALOGE("null path to write");
        return 0;
    }

    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        ALOGE("Could not open '%s'\n", path);
        char *err_str = strerror(errno);
        ALOGE("error : %d, %s\n", errno, err_str);
        return 0;
    }

    int count = write(fd, buf, size);
    if (count != size) {
        ALOGE("write file (%s,%s) fail, count: %d\n", path, buf, count);
        char *err_str = strerror(errno);
        ALOGE("error : %d, %s\n", errno, err_str);
        close(fd);
        return 0;
    }

    close(fd);
    return count;
}

/*
 *  return
 *      0: fail
 *      count: number of byte written
 */
int set_value(const char * path, const int value_1, const int value_2)
{
    char buf[32] = {0};
    if(snprintf(buf, sizeof(buf), "%d %d", value_1, value_2) < 0)
        return 0;
    return write_to_file(path, buf, strlen(buf));
}

/*
 *  return
 *      0: fail
 *      count: number of byte written
 */
int set_value(const char * path, const int value)
{
    char buf[32] = {0};
    if(snprintf(buf, sizeof(buf), "%d", value) < 0)
        return 0;
    return write_to_file(path, buf, strlen(buf));
}

void set_str_cpy(char * desc, const char *src, int desc_max_size)
{
    int len_sz = 0;

    ALOGI("set_str_cpy : 1");
    len_sz = strlen(src);
    ALOGI("set_str_cpy : 2");
    len_sz = (len_sz < desc_max_size) ? len_sz : (desc_max_size - 1);
    ALOGI("set_str_cpy : 3");
    strncpy(desc, src, len_sz);
    ALOGI("set_str_cpy : 4");
    desc[len_sz] = '\0';
}

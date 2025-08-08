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
#include <sys/ioctl.h>  /* ioctl */
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include "common.h"
//SPD: add powerencode by rui.zhou6 20250519 start
#include "base64.h"
//SPD: add powerencode by rui.zhou6 20250519 end
#include <string>
//SPD: add powerencode by rui.zhou6 20250519 start
int tran_powerhal_encode = 0;
void set_str_cpy(char * desc, const char *src, int desc_max_size)
{
    int len_sz = 0;
    len_sz = strlen(src);
    len_sz = (len_sz < desc_max_size) ? len_sz : (desc_max_size - 1);
    strncpy(desc, src, len_sz);
    desc[len_sz] = '\0';
}
int get_property_value(char *prop)
{
    char prop_content[PROPERTY_VALUE_MAX] = "\0";
    int prop_value = 0;
    if(prop == NULL)
        return 0;
    property_get(prop, prop_content, "0");
    prop_value = atoi(prop_content);
    return prop_value;
}
char* get_decode_buf(const char *path)
{
    size_t size = 0;
    FILE* file = NULL;
    char *buf = NULL;
    char *buf_decode = NULL;
    struct stat stat_buf;
    struct base64_decode_context ctx;
    if (0 == stat(path, &stat_buf)) {
        file = fopen(path, "r");
        buf = (char *)malloc(stat_buf.st_size);
        if(!buf) {
            ALOGE("malloc config failed");
            fclose(file);
            return NULL;
        }
        memset(buf, 0, stat_buf.st_size);
        size = fread(buf, 1, stat_buf.st_size, file);
        fclose(file);
        for(int i = 0; i < stat_buf.st_size; i++) {
            buf[i] -= 33;
        }
        buf_decode = (char *)malloc(stat_buf.st_size);
        if(!(buf_decode)) {
            ALOGE("malloc config failed");
            free(buf);
            return NULL;
        }
        size = stat_buf.st_size;
        base64_decode_ctx_init(&ctx);
        base64_decode_ctx(&ctx, buf, stat_buf.st_size, buf_decode, &size);
        free(buf);
    }
    return buf_decode;
}
//SPD: add powerencode by rui.zhou6 20250519 end
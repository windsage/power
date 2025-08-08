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

#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <base64.h>

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

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
#include "base64.h"
#include <string>

//#include <linux/disp_session.h>
#define MAX_CPU_NUM (32)

int devfdDSC = -1;

using namespace std;

int compare_descending(const void * arg1, const void * arg2)
{
    return ( *(int*)arg2 - *(int*)arg1 );
}

int compare_ascending(const void * arg1, const void * arg2)
{
    return ( *(int*)arg1 - *(int*)arg2 );
}

int compare_descending_64(const void * arg1, const void * arg2)
{
    return ( *(long long*)arg2 > *(long long*)arg1 ) ? 1 : -1;
}

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

int get_int_value(const char * path)
{
    int size = 32;
    char buf[32] = {0};
    if(!read_from_file(path, buf, size))
        return 0;
    return atoi(buf);
}

int get_cpu_num(void)
{
    int size = 32, cpu_first, cpu_last;
    char buf[32] = {0};
    if(!read_from_file(PATH_CPUNUM_POSSIBLE, buf, size))
        return 1;
    if (sscanf(buf, "%d-%d", &cpu_first, &cpu_last) < 2) {
        cpu_last = 0;
    }
    return (cpu_last + 1 > MAX_CPU_NUM) ? MAX_CPU_NUM : (cpu_last+1);
}

int get_cputopo_cpu_info(int cluster_num, int *p_cpu_num, int *p_first_cpu) // find first cpu of each cluster
{
    FILE *ifp = NULL;
    char  buf[128] = {0}, *str = NULL;
    int  i = 0, mask, cluster = 0, count, index;

    if ((ifp = fopen(PATH_PERFMGR_TOPO_CLUSTER_CPU,"r")) == NULL) {
        if ((ifp = fopen(PATH_CPUTOPO_CLUSTER_CPU,"r")) == NULL)
            return -1;
    }

    while(fgets(buf, 128, ifp) && cluster < cluster_num) {
        if (strlen(buf) < 3) // at least 3 characters, e.g., "a b"
            continue;

        str = strtok(buf, " ");
        if (str == NULL) {
            if(fclose(ifp) == EOF)
                ALOGE("fclose errno:%d", errno);
            return -1;
        }
        //ALOGI("str : %s", str);
        str = strtok(NULL, " ");
        if (str == NULL) {
            if(fclose(ifp) == EOF)
                ALOGE("fclose errno:%d", errno);
            return -1;
        }
        //ALOGI("str : %s", str);
        if (sscanf(str, "%x", &mask) < 1) {
            if(fclose(ifp) == EOF)
                ALOGE("fclose errno:%d", errno);
            return -1;
        }
        //ALOGI("mask : %d, %x", mask, mask);

        count = 0;
        index = -1;
        for(i=0; mask>0; i++) {
            if (mask % 2 == 1) {
                if (index == -1)
                    index = i;
                count++;
            }
            mask /= 2;
        }
        p_cpu_num[cluster] = count;
        p_first_cpu[cluster] = index;

        cluster++;
    }

    if(fclose(ifp) == EOF)
        ALOGE("fclose errno:%d", errno);
    return 0;
}


void get_task_comm(const char *path, char *comm)
{
    int size = 64;
    char buf[64] = {0};
    if(!read_from_file(path, buf, size))
        comm[0] = '\0';
    else
        set_str_cpy(comm, buf, size);
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
    if(sprintf(buf, "%d %d", value_1, value_2) < 0)
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
    if(sprintf(buf, "%d", value) < 0)
        return 0;
    return write_to_file(path, buf, strlen(buf));
}

/*
 *  return
 *      0: fail
 *      count: number of byte written
 */
int set_value(const char * path, const long long value)
{
    char buf[32] = {0};
    if(sprintf(buf, "%lld", value) < 0)
        return 0;
    return write_to_file(path, buf, strlen(buf));
}

/*
 *  return
 *      0: fail
 *      count: number of byte written
 */
int set_value(const char * path, const char *str)
{
    return write_to_file(path, str, strlen(str));
}

/*
 *  return
 *      0: fail
 *      count: number of byte written
 */
int set_value(const char * path, const string *str)
{
    return write_to_file(path, str->c_str(), str->length());
}

void get_str_value(const char * path, char *str, int len)
{
    read_from_file(path, str, len);
}

void set_str_cpy(char * desc, const char *src, int desc_max_size)
{
    int len_sz = 0;

    len_sz = strlen(src);
    len_sz = (len_sz < desc_max_size) ? len_sz : (desc_max_size - 1);
    strncpy(desc, src, len_sz);
    desc[len_sz] = '\0';
}

void get_ppm_cpu_freq_info(int cluster_index, int *p_max_freq, int *p_count, int **pp_table) // max freq, freq level counts, freq table
{
    char file[128] = {0}, *str = NULL, buf[256] = {0};
    int *tbl = NULL, count=0, i=0;

    if(sprintf(file, "/proc/ppm/dump_cluster_%d_dvfs_table", cluster_index) < 0)
        return;
    if(!read_from_file(file, buf, sizeof(buf)))
        return;

    str = strtok(buf, " ");
    while(str) {
        count++;
        str = strtok(NULL, " ");
    }

    *p_count = count;
    if(count <= 0) return;

    /* create table */
    *pp_table = (int*)malloc(sizeof(int)*count);
    if (*pp_table == NULL)
        return;

    tbl = (int*)malloc(sizeof(int)*count);
    if(tbl == NULL)
        return;

    for(i=0; i<count; i++)
        tbl[i] = 0;

    if(!read_from_file(file, buf, sizeof(buf)))
        goto ERR_EXIT;

    str = strtok(buf, " ");
    i = 0;
    while(str && i<count) {
        tbl[i] = atoi(str);
        i++;
        str = strtok(NULL, " ");
    }

    for(i=0; i<count; i++)
        (*pp_table)[i] = tbl[count-1-i];
    *p_max_freq = tbl[0];

ERR_EXIT:
    if(tbl != NULL)
        free(tbl);
}


void get_cpu_freq_info(int cpu_index, int *p_max_freq, int *p_count, int **pp_table) // max freq, freq level counts, freq table
{
    char file[128] = {0}, *str = NULL, buf[256] = {0};
    int *tbl = NULL, count=0, i=0;

#if 0
    sprintf(file, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu_index);
    while((*p_max_freq = get_int_value(file)) == 0) {
        usleep(4000);
        if(timeout++ > 50) {
            ALOGI("get_cpu_freq_info:%d, timeout!!!", cpu_index);
            break;
        }
    }
#endif

    if(sprintf(file, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", cpu_index) < 0)
        return;
    if(!read_from_file(file, buf, sizeof(buf)))
        return;

    str = strtok(buf, " ");
    while(str) {
        count++;
        str = strtok(NULL, " ");
    }

    *p_count = count;
    if(count <= 0) return;

    /* create table */
    *pp_table = (int*)malloc(sizeof(int)*count);
    if (*pp_table == NULL)
        return;

    tbl = (int*)malloc(sizeof(int)*count);
    if (tbl == NULL)
        return;

    if(!read_from_file(file, buf, sizeof(buf)))
        goto ERR_EXIT;

    str = strtok(buf, " ");
    while(str) {
        tbl[i] = atoi(str);
        i++;
        str = strtok(NULL, " ");
    }

    qsort(tbl, count, sizeof(int), compare_descending);
    for(i=0; i<count; i++)
        (*pp_table)[i] = tbl[count-1-i];

    /* workaround */
    /*if(*p_max_freq == 0)*/
    if((*p_max_freq) != ((*pp_table)[count - 1]))
        *p_max_freq = (*pp_table)[count - 1];

ERR_EXIT:
    if(tbl != NULL)
        free(tbl);
}

void getCputopoFromSysfs(int cpuNum, int *pnClusterNum, int *p_cpu_num, int *p_first_cpu)
{
    DIR *d;
    struct dirent *de;
    int policy = 0;
    int id, i;

    if(pnClusterNum == NULL)
        return;

    d = opendir(PATH_CPU_CPUFREQ);
    if(d == 0) return;
    while((de = readdir(d)) != 0){
        ALOGD("%s", de->d_name);
        if (strncmp("policy", de->d_name, 6) == 0) {
            if (sscanf(de->d_name, "policy%d", &id) < 1) {
                ALOGE("sscanf error");
                break;
            }

            if (p_first_cpu != NULL) {
                p_first_cpu[policy] = id;
            }

            policy++;
        }
    }

    //ALOGI("policy:%d", policy);
    *pnClusterNum = policy;

    if (policy > 0 && p_first_cpu != NULL && p_cpu_num != NULL) {
        qsort(p_first_cpu, policy, sizeof(int), compare_ascending);

        for (i=1; i<policy; i++) {
            p_cpu_num[i-1] = p_first_cpu[i] - p_first_cpu[i-1];
            //ALOGI("cpu_num[%d] = %d", (i-1), p_cpu_num[i-1]);
        }

        p_cpu_num[i-1] = cpuNum - p_first_cpu[i-1];
        //ALOGI("cpu_num[%d] = %d", (i-1), p_cpu_num[i-1]);
    }

    closedir(d);
    return;
}


void get_gpu_freq_level_count(int *p_count)
{
    *p_count = get_int_value(PATH_GPUFREQ_COUNT);
    //ALOGI("get_gpu_freq_level_count:%d", *p_count);
}

void set_gpu_freq_level(int level)
{
    char buf[32];
    if(sprintf(buf, "%d", level) < 0)
        return;
    write_to_file(PATH_GPUFREQ_BASE, buf, strlen(buf));
}

void set_gpu_freq_level_max(int level)
{
    char buf[32];
    if(sprintf(buf, "%d", level) < 0)
        return;
    write_to_file(PATH_GPUFREQ_MAX, buf, strlen(buf));
}

void get_dvfsrc_devfreq_table(int *freqCount, long long **ppFreqTbl)
{
    struct stat stat_buf;
    char buf[512] = {0}, *str = NULL;
    int count = 0, i = 0;

    if (freqCount == NULL || ppFreqTbl == NULL)
        return;

    *freqCount = 0;
    if (stat(DVFSRC_DDR_OPP_TABLE, &stat_buf) != 0)
        return;

    if(!read_from_file(DVFSRC_DDR_OPP_TABLE, buf, sizeof(buf)))
        return;

    str = strtok(buf, " ");
    while(str) {
        count++;
        str = strtok(NULL, " ");
    }

    ALOGI("get_dvfsrc_devfreq_table count:%d", count);
    if(count <= 0) return;

    /* create table */
    *freqCount = count;
    *ppFreqTbl = (long long*)malloc(sizeof(long long)*count);
    if (*ppFreqTbl == NULL)
        return;

    if(!read_from_file(DVFSRC_DDR_OPP_TABLE, buf, sizeof(buf))) {
        free(*ppFreqTbl);
        *freqCount = 0;
        return;
    }

    str = strtok(buf, " ");
    while(str && i<count) {
        (*ppFreqTbl)[i] = atoll(str);
        //ALOGD("get_dvfsrc_drefreq_table %d:%lld", i, (*ppFreqTbl)[i]);
        i++;
        str = strtok(NULL, " ");
    }

    qsort(*ppFreqTbl, count, sizeof(long long), compare_descending_64);
    for(i=0; i<count; i++)
        ALOGI("dvfsrc devfreq[%d]: %lld", i, (*ppFreqTbl)[i]);
    return ;
}

//SPD: add powerencode by rui.zhou6 20240412 start
int tran_powerhal_encode = 0;

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

//SPD: add powerencode by rui.zhou6 20240412 end
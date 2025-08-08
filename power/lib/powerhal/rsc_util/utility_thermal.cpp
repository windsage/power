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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <dlfcn.h>
#include <string.h>

#include "perfservice.h"
#include "utility_thermal.h"
#include "utility_fps.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


using namespace std;

#define MAX_POLICY      (23) /* 3 for PowerHAL and 20 for specific scenarios. */
#if defined (__LP64__) ||  defined (_LP64)
#define THM_LIB_FULL_NAME  "/vendor/lib64/libmtcloader.so"
#else
#define THM_LIB_FULL_NAME  "/vendor/lib/libmtcloader.so"
#endif

#define THERMAL_CORE_SOCKET_NAME	"/dev/socket/thermal_socket"
#define FRS_SOCKET_NAME	            "/dev/socket/frs_socket"
#define BUFFER_SIZE 	(128)
#define POLICY_PROP_NAME        "vendor.thermal.manager.data.tp.ref"
#define POLICY_PROP_MAX_NUM     3
#define POLICY_PROP_VALUE_MAX   (PROPERTY_VALUE_MAX - 3)
#define POLICY_PROP_ITEM_MAX    (POLICY_PROP_VALUE_MAX/4)

static int load_thm_api(int idx, bool start);

thermal_conf_type g_perf_tc[MAX_POLICY] = {
    [0] = {.conf_name = "thermal_sp",.perf_used = 0},
    [1] = {.conf_name = "thermal_vr",.perf_used = 0},
    [2] = {.conf_name = "thermal_vrsp",.perf_used = 0},
    [3] = {.conf_name = "thermal_policy_00",.perf_used = 0},
    [4] = {.conf_name = "thermal_policy_01",.perf_used = 0},
    [5] = {.conf_name = "thermal_policy_02",.perf_used = 0},
    [6] = {.conf_name = "thermal_policy_03",.perf_used = 0},
    [7] = {.conf_name = "thermal_policy_04",.perf_used = 0},
    [8] = {.conf_name = "thermal_policy_05",.perf_used = 0},
    [9] = {.conf_name = "thermal_policy_06",.perf_used = 0},
    [10] = {.conf_name = "thermal_policy_07",.perf_used = 0},
    [11] = {.conf_name = "thermal_policy_08",.perf_used = 0},
    [12] = {.conf_name = "thermal_policy_09",.perf_used = 0},
    [13] = {.conf_name = "thermal_policy_10",.perf_used = 0},
    [14] = {.conf_name = "thermal_policy_11",.perf_used = 0},
    [15] = {.conf_name = "thermal_policy_12",.perf_used = 0},
    [16] = {.conf_name = "thermal_policy_13",.perf_used = 0},
    [17] = {.conf_name = "thermal_policy_14",.perf_used = 0},
    [18] = {.conf_name = "thermal_policy_15",.perf_used = 0},
    [19] = {.conf_name = "thermal_policy_16",.perf_used = 0},
    [20] = {.conf_name = "thermal_policy_17",.perf_used = 0},
    [21] = {.conf_name = "thermal_policy_18",.perf_used = 0},
    [22] = {.conf_name = "thermal_policy_19",.perf_used = 0}
};

static struct policy_status_t policy_status;

static thermal_policy_manager_type g_tpm;
static int thm_get_next_conf(const char *conf_name, int enable)
{
    int i, found = 0;
    int ret = TS_ERR_NO_LOAD;

    if (strnlen(conf_name, MAX_CONF_NAME + 1) > MAX_CONF_NAME)
        return TS_ERR_NO_FILE_EXIST;



    if (enable) { /* enable policy */
    for (i = 0; i < MAX_POLICY; i++) {
            if (!strcmp(conf_name, g_perf_tc[i].conf_name)) {
                LOG_D("enable %s.\n", g_perf_tc[i].conf_name);
                g_perf_tc[i].perf_used++;
                g_tpm.policy_count++;
                if (i > g_tpm.policy_active || ((g_tpm.policy_active == i) && (i == 0))) {
                    g_tpm.policy_active = i;
                    ret = i;
                    return ret;
                }
            }
        }
    } else { /* disable policy */
        for (i = 0; i < MAX_POLICY; i++) {
            if (!strcmp(conf_name, g_perf_tc[i].conf_name)) {
                    if (g_perf_tc[i].perf_used > 0) {
                        g_perf_tc[i].perf_used--;
                    if (g_tpm.policy_count > 0)
                        g_tpm.policy_count--;

                    LOG_D("disable %s.\n", g_perf_tc[i].conf_name);
                }
                found = 1;
                break;
            }
        }
        if (found) {
            if (0 == g_tpm.policy_count)
                g_tpm.policy_active = -1;

            for (i = MAX_POLICY-1; i >= 0; i--) {
                if (g_perf_tc[i].perf_used > 0) {
                    ret = i;
                    g_tpm.policy_active = i;
                    break;
                }
            }
        } else {
            return ret;
        }
    }

    return ret;
}

static int encode_policy_prop(unsigned int start, unsigned int len, policy_ref_t *policy_list, char *str)
{
    unsigned int i, idx = 0;
    int size = 0;
    policy_ref_t *ptr;

    if (!policy_list)
        return 0;

    if (len > POLICY_PROP_ITEM_MAX) {
        LOG_E("error input %u, %u\n", start, len);
        return -1;
    }

    ptr = policy_list;

    for (i = 0; i < (start + len); i ++) {
        if (!ptr) {
            LOG_E("NULL ptr %u\n", i);
            return -1;
        }

        if (i >= start) {
            if (ptr->id > 999 || ptr->refcnt > 999) {
                LOG_E("invalid id=%d, refcnt=%d\n", ptr->id, ptr->refcnt);
                return -1;
            }

            if (idx >= POLICY_PROP_VALUE_MAX - 3) {
                LOG_E("str will overflow %d\n", idx);
                return -1;
            }

            /* Since policy idx and refcnt range are 0-999
             * encode each number to 2 bytes to save capacity for storing in prpoerty
             */
            str[idx] = (ptr->id >> 5) + 0x30;
            str[idx+1] = (ptr->id & 0x1F) + 0x30;
            str[idx+2] = (ptr->refcnt >> 5) + 0x30;
            str[idx+3] = (ptr->refcnt & 0x1F) + 0x30;
            idx = idx + 4;
            size = size + 1;
        }

        ptr = ptr->next;
    }
    return size;
}

static int decode_policy_prop(struct policy_ref_t *policy_ref, char *str)
{
    unsigned int idx = 0;

    if (strlen(str) < 4) {
        LOG_E("strlen should larger than 4 len=%zu", strlen(str));
        return -1;
    }
    policy_ref->id = (((str[idx] - 0x30) << 5) & 0x3E0) | (str[idx+1] - 0x30);
    policy_ref->refcnt = (((str[idx+2] - 0x30) << 5) & 0x3E0) | (str[idx+3] - 0x30);

    return 0;
}

static int get_next_policy(void)
{
    struct policy_ref_t *cur;
    int ret = -1;

    cur = policy_status.list;
    while (cur != NULL) {
        if ((int)cur->id >= ret)
            ret = (int)cur->id;

        cur = cur->next;
    }

    return ret;
}

static unsigned int get_policy_refcnt(unsigned int id)
{
    struct policy_ref_t *cur;
    unsigned int ret = 0;

    cur = policy_status.list;
    while (cur != NULL) {
        if (cur->id == id) {
            ret = cur->refcnt;
            break;
        }
        cur = cur->next;
    }

    return ret;
}

static int add_policy_list(unsigned int id, unsigned int refcnt)
{
    struct policy_ref_t *cur;

    cur = (struct policy_ref_t *)malloc(sizeof(struct policy_ref_t));
    if (!cur) {
        LOG_E("out of memory\n");
        return -1;
    }
    cur->id = id;
    cur->refcnt = refcnt;
    cur->next = policy_status.list;
    policy_status.list = cur;

    if ((int)(policy_status.max_id - id) < 0)
        policy_status.max_id = (int)id;

    policy_status.list_len++;

    return 0;
}

static int remove_policy_list(unsigned int id)
{
    struct policy_ref_t *cur, *pre;

    cur = policy_status.list;
    pre = policy_status.list;

    while (cur != NULL) {
        if (cur->id == id) {
            if (cur == policy_status.list)
                policy_status.list = cur->next;
            else
                pre->next = cur->next;

            if (policy_status.max_id == (int)id)
                policy_status.max_id = get_next_policy();

            policy_status.list_len--;
            free(cur);
            return 0;
        }
        pre = cur;
        cur = cur->next;
    }

    return -1;
}

static int update_policy_list(unsigned int id, unsigned int refcnt)
{
    struct policy_ref_t *cur;
    int ret = 0;

    cur = policy_status.list;

    while (cur != NULL) {
        if (cur->id == id) {
            if (refcnt == 0) {
                remove_policy_list(id);
                return 0;
            }

            cur->refcnt = refcnt;
            return 0;
        }
        cur = cur->next;
    }

    if (!cur)
        ret = add_policy_list(id, refcnt);

    return ret;
}

static int safety_snprintf(char * str, size_t size, const char * fromat)
{
    int check;
    check = snprintf(str, size, "%s", fromat);

    if (check< 0 || check == size) {
        LOG_V("sprintf fail");
        return -1;
    }
    return check;
}

template <typename T, typename... Ts>
static int safety_snprintf(char * str, size_t size, const char * fromat, T head, Ts... Tail)
{
    int check;
    check = snprintf(str, size, fromat, head, Tail...);

    if (check< 0 || check == size) {
        LOG_V("sprintf fail");
        return -1;
    }
    return check;
}

static void dump_policy_list(void)
{
    struct policy_ref_t *cur;
    int idx = 0, len = 0, ret = 0;
    char str[128];

    cur = policy_status.list;

    while (cur != NULL) {
        ret = safety_snprintf(str + len, 128 - len, "[%3d,%3d] ", cur->id, cur->refcnt);

        if( ret == -1 )
            continue;

        len += ret;
        cur = cur->next;
        idx++;
        if (idx % 10 == 0 || cur == NULL) {
            LOG_I("%s", str);
            len = 0;
            memset(str, 0, 128);
        }
    }
}

static int load_policy_prop(void)
{
    int ret = -1;
    unsigned int num;
    char prop_s[PROPERTY_VALUE_MAX] = {0};
    char policy_s[POLICY_PROP_VALUE_MAX] = {0};
    char prop_name[64];
    struct policy_ref_t policy_ref;
    unsigned int i, j;

    if (policy_status.list)
		return 0;

    /* each property can store 22 items, define as POLICY_PROP_ITEM_MAX
     * user can define how many property to store by POLICY_PROP_MAX_NUM, like 3
     * max policy items with refcnt>0 are 22 * 3 = 66
     */
    for(i = 0; i < POLICY_PROP_MAX_NUM; i++) {
        ret = safety_snprintf(prop_name, 64, "%s%u", POLICY_PROP_NAME, i);
        if (ret < 0) {
            LOG_E("copy prop_name error ret=%d", ret);
            return -1;
        }

        if (!property_get(prop_name, prop_s, NULL)) {
            LOG_V("can't get property %s", prop_name);
            break;
        }

        if(sscanf(prop_s, "%d %88s", &num, policy_s) < 2) {
            LOG_V("can't scan property %s", prop_s);
            break;
        }

        for (j = 0; j < num; j++) {
            ret = decode_policy_prop(&policy_ref, &policy_s[j*4]);
            if (ret < 0) {
                LOG_E("decode prop error %d:%s", ret, policy_s);
                return -1;
            }

            update_policy_list(policy_ref.id, policy_ref.refcnt);
        }

        if (num < POLICY_PROP_ITEM_MAX)
            break;
    }

    return 0;
}

static int save_policy_prop(void)
{
    int ret = -1;
    char prop_s[PROPERTY_VALUE_MAX];
    char policy_s[POLICY_PROP_VALUE_MAX] = {0};
    char prop_name[64];
    unsigned int enc_num = 0, total_num = 0;
    unsigned int i = 0;
    unsigned int start = 0, len = 0;

    total_num = policy_status.list_len;

    for (i = 0; i < POLICY_PROP_MAX_NUM; i++){
        memset(policy_s, 0, POLICY_PROP_VALUE_MAX);
        start = enc_num;
        len = ((total_num - enc_num) > POLICY_PROP_ITEM_MAX) ?
            POLICY_PROP_ITEM_MAX : (total_num - enc_num);

        ret = encode_policy_prop(start, len, policy_status.list, policy_s);
        if (ret < 0)
            return ret;
        ret = safety_snprintf(prop_s, PROPERTY_VALUE_MAX, "%d %s", len, policy_s);
        if (ret < 0) {
            LOG_E("copy prop error, ret=%d num=%d, policy_s=%s", ret, total_num, policy_s);
            return ret;
        }

        enc_num = enc_num + len;

        ret = safety_snprintf(prop_name, 64, "%s%u", POLICY_PROP_NAME, i);
        if (ret < 0)
            LOG_E("copy prop_name error ret=%d, i=%d", ret, i);

        ret = property_set(prop_name, prop_s);
        LOG_I("property_set %s=%s\n", prop_name, prop_s);
        if (enc_num >= total_num)
            break;
    }

    return ret;
}

static int thm_load_thm_data(void)
{
    int ret = -1;
    char value[PROPERTY_VALUE_MAX] = {0};

    ret = property_get("vendor.thermal.manager.data.perf", value, "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    if(sscanf(value,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
        &g_perf_tc[0].perf_used, &g_perf_tc[1].perf_used, &g_perf_tc[2].perf_used, &g_perf_tc[3].perf_used, &g_perf_tc[4].perf_used,
        &g_perf_tc[5].perf_used, &g_perf_tc[6].perf_used, &g_perf_tc[7].perf_used, &g_perf_tc[8].perf_used, &g_perf_tc[9].perf_used,
        &g_perf_tc[10].perf_used, &g_perf_tc[11].perf_used, &g_perf_tc[12].perf_used, &g_perf_tc[13].perf_used,&g_perf_tc[14].perf_used,
        &g_perf_tc[15].perf_used, &g_perf_tc[16].perf_used, &g_perf_tc[17].perf_used, &g_perf_tc[18].perf_used,&g_perf_tc[19].perf_used,
        &g_perf_tc[20].perf_used, &g_perf_tc[21].perf_used, &g_perf_tc[22].perf_used) < 23) {
        LOG_E("sscanf error");
    }

    LOG_D("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        g_perf_tc[0].perf_used, g_perf_tc[1].perf_used, g_perf_tc[2].perf_used, g_perf_tc[3].perf_used, g_perf_tc[4].perf_used,
        g_perf_tc[5].perf_used, g_perf_tc[6].perf_used, g_perf_tc[7].perf_used, g_perf_tc[8].perf_used, g_perf_tc[9].perf_used,
        g_perf_tc[10].perf_used, g_perf_tc[11].perf_used, g_perf_tc[12].perf_used, g_perf_tc[13].perf_used, g_perf_tc[14].perf_used,
        g_perf_tc[15].perf_used, g_perf_tc[16].perf_used, g_perf_tc[17].perf_used, g_perf_tc[18].perf_used, g_perf_tc[19].perf_used,
        g_perf_tc[20].perf_used, g_perf_tc[21].perf_used, g_perf_tc[22].perf_used);

    return ret;

}

static int thm_store_thm_data(void)
{
    int ret = -1;
    char value[PROPERTY_VALUE_MAX];

    LOG_D("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        g_perf_tc[0].perf_used, g_perf_tc[1].perf_used, g_perf_tc[2].perf_used, g_perf_tc[3].perf_used, g_perf_tc[4].perf_used,
        g_perf_tc[5].perf_used, g_perf_tc[6].perf_used, g_perf_tc[7].perf_used, g_perf_tc[8].perf_used, g_perf_tc[9].perf_used,
        g_perf_tc[10].perf_used, g_perf_tc[11].perf_used, g_perf_tc[12].perf_used, g_perf_tc[13].perf_used, g_perf_tc[14].perf_used,
        g_perf_tc[15].perf_used, g_perf_tc[16].perf_used, g_perf_tc[17].perf_used, g_perf_tc[18].perf_used, g_perf_tc[19].perf_used,
        g_perf_tc[20].perf_used, g_perf_tc[21].perf_used, g_perf_tc[22].perf_used);

    if(snprintf(value, sizeof(value), "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
        g_perf_tc[0].perf_used, g_perf_tc[1].perf_used, g_perf_tc[2].perf_used, g_perf_tc[3].perf_used, g_perf_tc[4].perf_used,
        g_perf_tc[5].perf_used, g_perf_tc[6].perf_used, g_perf_tc[7].perf_used, g_perf_tc[8].perf_used, g_perf_tc[9].perf_used,
        g_perf_tc[10].perf_used, g_perf_tc[11].perf_used, g_perf_tc[12].perf_used, g_perf_tc[13].perf_used, g_perf_tc[14].perf_used,
        g_perf_tc[15].perf_used, g_perf_tc[16].perf_used, g_perf_tc[17].perf_used, g_perf_tc[18].perf_used, g_perf_tc[19].perf_used,
        g_perf_tc[20].perf_used, g_perf_tc[21].perf_used, g_perf_tc[22].perf_used) < 0) {
        LOG_E("sprintf error");
        return -1;
    }

    ret = property_set("vendor.thermal.manager.data.perf", value);


    return ret;
}

int load_thm_api_start(int idx, void *scn)
{
    LOG_V("%p", scn);
    return load_thm_api(idx, 1);
}

int load_thm_api_stop(int idx, void *scn)
{
    LOG_V("%p", scn);
    return load_thm_api(idx, 0);
}

static int send_by_socket_thermal_core(char *cmd, int sz)
{
	struct sockaddr_un addr;
	int client_fd, ret;

	client_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (client_fd == -1) {
		LOG_E("Failed to create a local socket\n");
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, THERMAL_CORE_SOCKET_NAME, sizeof(addr.sun_path) - 1);

	ret = connect(client_fd, (const struct sockaddr *) &addr,
			sizeof(struct sockaddr_un));
	if (ret == -1) {
		LOG_E("Server not found\n");
		close(client_fd);
		return -1;
	}

	ret = send(client_fd, cmd, sz,0);
	if (ret == -1)
		LOG_E("Socket send failed: %s\n", strerror(errno));

	close(client_fd);
	return 0;
}

int send_by_socket(char *cmd, int sz)
{
	struct sockaddr_un addr;
	int client_fd, ret;

	client_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (client_fd == -1) {
		LOG_E("Failed to create a local socket\n");
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, FRS_SOCKET_NAME, sizeof(addr.sun_path) - 1);

	ret = connect(client_fd, (const struct sockaddr *) &addr,
			sizeof(struct sockaddr_un));
	if (ret == -1) {
		LOG_E("Server not found\n");
		close(client_fd);
		return -1;
	}

	ret = send(client_fd, cmd, sz,0);
	if (ret == -1)
		LOG_E("Socket send failed: %s\n", strerror(errno));

	close(client_fd);
	return 0;
}

int frs_setting(int change_en, int enable, int change_th, int threshold, int change_tt, int target_temp)
{
	int ret = 0;
	char buffer[BUFFER_SIZE];

	if (change_en) {
		safety_snprintf(buffer, sizeof(buffer), "frs enable %d %d", enable, get_fpsgo_render_pid());
		ret = send_by_socket(buffer, sizeof(buffer));
		if (ret < 0)
			return ret;
	}
	if (change_th) {
		safety_snprintf(buffer, sizeof(buffer), "frs threshold %d", threshold);
		ret = send_by_socket(buffer, sizeof(buffer));
		if (ret < 0)
			return ret;
	}
	if (change_tt) {
		safety_snprintf(buffer, sizeof(buffer), "frs target_temp %d", target_temp);
		ret = send_by_socket(buffer, sizeof(buffer));
		if (ret < 0)
			return ret;
	}
	return ret;
}

int fps_target_fps(int change_max, int max_fps, int change_min, int min_fps)
{
	 int ret = 0;
    char buffer[BUFFER_SIZE];

    if (change_max) {
        safety_snprintf(buffer, sizeof(buffer), "frs max_fps %d", max_fps);
        ret = send_by_socket(buffer, sizeof(buffer));
        if (ret < 0)
            return ret;
    }
    if (change_min) {
        safety_snprintf(buffer, sizeof(buffer), "frs min_fps %d", min_fps);
        ret = send_by_socket(buffer, sizeof(buffer));
        if (ret < 0)
            return ret;
    }
	return ret;
}

int fps_ceiling(int change_ceiling, int ceiling, int change_ceiling_th, int ceiling_th)
{
    int ret = 0;
    char buffer[BUFFER_SIZE];

    if (change_ceiling) {
        safety_snprintf(buffer, sizeof(buffer), "frs ceiling %d", ceiling);
        ret = send_by_socket(buffer, sizeof(buffer));
        if (ret < 0)
            return ret;
    }
    if (change_ceiling_th) {
        safety_snprintf(buffer, sizeof(buffer), "frs ceiling_th %d", ceiling_th);
        ret = send_by_socket(buffer, sizeof(buffer));
        if (ret < 0)
            return ret;
    }
	return ret;
}

int frs_set(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(1, idx, 0, -1, 0, -1);
}

int frs_unset(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(1, !idx, 0, -1, 0, -1);
}

int frs_threshold(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(0, -1, 1, idx, 0, -1);
}

int frs_unset_threshold(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(0, -1, 1, -1, 0, -1);
}

int frs_target_temp(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(0, -1, 0, -1, 1, idx);
}

int frs_unset_target_temp(int idx, void *scn)
{
    LOG_V("%p", scn);
    return frs_setting(0, -1, 0, -1, 1, -1);
}

int frs_max_fps(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps(1, idx, 0, -1);
}

int frs_unset_max_fps(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps(1, -1, 0, -1);
}

int frs_min_fps(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps(0, -1, 1, idx);
}

int frs_unset_min_fps(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps(0, -1, 1, -1);
}

int frs_ceiling(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_ceiling(1, idx, 0, -1);
}

int frs_unset_ceiling(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_ceiling(1, -1, 0, -1);
}

int frs_ceiling_th(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_ceiling(0, -1, 1, idx);
}

int frs_unset_ceiling_th(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_ceiling(0, -1, 1, -1);
}

int frs_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ptime %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ptime -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_ot_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ot_ptime %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_ot_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ot_ptime -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_throttled(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs throttled %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_throttled(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs throttled -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_ot_throttled(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ot_throttled %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_ot_throttled(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ot_throttled -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_max_throttled_fps(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs max_throttled_fps %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_max_throttled_fps(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs max_throttled_fps -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_fps_ratio(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs fps_ratio %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_fps_ratio(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs fps_ratio -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_fps_ratio_step(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs fps_ratio_step %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_fps_ratio_step(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs fps_ratio_step -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_mode(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs mode %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_mode(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs mode -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_set_reset(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs reset %d", idx);

    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_reset(int idx, void *scn) {

	/* DO nothing */
    return 0;
}

static int fps_target_fps_diff(int max_fps_diff)
{
    int ret = 0;
    char buffer[BUFFER_SIZE];
    int check;

    if(safety_snprintf(buffer, sizeof(buffer), "frs max_fps_diff %d", max_fps_diff) == -1){
        return -1;
    }
    ret = send_by_socket(buffer, sizeof(buffer));

    return ret;
}

int frs_max_fps_diff(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps_diff(idx);
}

int frs_unset_max_fps_diff(int idx, void *scn)
{
    LOG_V("%p", scn);
    return fps_target_fps_diff(-1);
}

int frs_ur_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ur_ptime %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_ur_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ur_ptime -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_urgent_temp_offset(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs urgent_temp_offset %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_urgent_temp_offset(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs urgent_temp_offset -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_tz_idx(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs tz_idx %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_tz_idx(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs tz_idx -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_sys_ceiling(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs sys_ceiling %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_sys_ceiling(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs sys_ceiling -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_cold_threshold(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs cold_threshold %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_cold_threshold(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs cold_threshold -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_limit_cfreq(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_cfreq %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_limit_cfreq(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_cfreq -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_limit_rfreq(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_rfreq %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_limit_rfreq(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_rfreq -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_limit_cfreq_m(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_cfreq_m %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_limit_cfreq_m(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_cfreq_m -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_limit_rfreq_m(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_rfreq_m %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_limit_rfreq_m(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs limit_rfreq_m -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_ct_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ct_ptime %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_ct_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs ct_ptime -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_min_fps_target_temp(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs min_fps_target_temp %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_min_fps_target_temp(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    if(safety_snprintf(buffer, sizeof(buffer), "frs min_fps_target_temp -1") == -1){
        return -1;
    }
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_1 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_1 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_2 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_2 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_3 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_3 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_4 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_4 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_5 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_5 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_6 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_6 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_7 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_7 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster0_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_8 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster0_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c0_8 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_1 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_1 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_2 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_2 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_3 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_3 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_4 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_4 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_5 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_5 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_6 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_6 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_7 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_7 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster1_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_8 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster1_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c1_8 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_1 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_1(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_1 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_2 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_2(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_2 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_3 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_3(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_3 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_4 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_4(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_4 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_5 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_5(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_5 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_6 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_6(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_6 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_7 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_7(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_7 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_cluster2_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_8 %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_cluster2_8(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_c2_8 -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_hysteresis(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_hysteresis %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_hysteresis(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_hysteresis -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_ptime %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}
int frs_unset_freq_temp_mapping_ptime(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_ptime -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_freq_temp_mapping_tz_idx(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_tz_idx %d", idx);
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int frs_unset_freq_temp_mapping_tz_idx(int idx, void *scn) {
    int ret = 0;
    char buffer[BUFFER_SIZE];

    safety_snprintf(buffer, sizeof(buffer), "frs freq_temp_tz_idx -1");
    ret = send_by_socket(buffer, sizeof(buffer));
    return ret;
}

int change_policy_by_socket(char *conf_name, int enable)
{
	char buffer[BUFFER_SIZE];
	int ret;

	if (enable > 0)
		safety_snprintf(buffer, sizeof(buffer), "apply %s.conf", conf_name);
	else
		safety_snprintf(buffer, sizeof(buffer), "apply %s", "thermal.conf");/*default policy*/

	ret = send_by_socket_thermal_core(buffer, sizeof(buffer));
	return ret;
}

static int load_thm_api(int idx, bool start)
{
    void *func, *handle = nullptr;
    char conf_name[MAX_CONF_NAME] = {0};
    int ret = 0;
    unsigned int refcnt, len;
    int next_index = -2; /* -1 is unnecessary to load conf */
    char value[PROPERTY_VALUE_MAX] = "\0";
    char *endptr;
    long val;

    LOG_I("idx:%d, start:%d", idx, start);

    property_get("ro.vendor.mtk_thermal_2_0", value, "0");

    if (idx > 999 || idx < 0) {
        LOG_E("idx out of range: %d", idx);
        return -1;
    }

    if (safety_snprintf(conf_name, sizeof(conf_name), "thermal_policy_%02d", idx) < 0) {
        LOG_E("snprintf error");
        return -1;
    }

    errno = 0;
    val = strtol(value, &endptr, 10);
    if (errno != 0) {
        LOG_E("strtol error, errno: %d\n", errno);
        return -1;
    }

    if (val == 1) {
        load_policy_prop();
        refcnt = get_policy_refcnt(idx);
        len = policy_status.list_len;

        if (start) {
            if (refcnt == 999) {
                LOG_E("id=%d refcnt=%d already max", idx, refcnt);
                return -1;
            }
            if (len >= (POLICY_PROP_MAX_NUM * POLICY_PROP_ITEM_MAX) && refcnt == 0) {
                LOG_E("policy list length %d over limit", len);
                return -1;
            }
            refcnt++;
        } else {
            if (refcnt == 0) {
                LOG_E("id=%d refcnt=%d shouldn't be 0", idx, refcnt);
                return -1;
            }
            refcnt--;
        }

        update_policy_list(idx, refcnt);
        dump_policy_list();
        next_index = policy_status.max_id;
        save_policy_prop();

        if (next_index == policy_status.active_policy) {
            LOG_I("policy=%d, no change", next_index);
            return 0;
        }

        if (next_index >= 0) {
            if (safety_snprintf(conf_name, sizeof(conf_name), "thermal_policy_%02d", next_index) < 0) {
                LOG_E("copy for thermal_policy error");
                return -1;
            }
        } else if (next_index == -1) {
            if (safety_snprintf(conf_name, sizeof(conf_name), "thermal") < 0) {
                LOG_E("copy for thermal error");
                return -1;
            }
        }

        ret = change_policy_by_socket(conf_name, 1);
        if (ret == 0)
            policy_status.active_policy = next_index;

        return ret;
    }

    handle = dlopen(THM_LIB_FULL_NAME, RTLD_NOW);

    if (handle == NULL) {
        LOG_E("Can't load library: %s", dlerror());
        return -1;
    }

    func = dlsym(handle, "change_policy");
    if(func == NULL) {
        LOG_E("dlsym fail:%s", dlerror());
        dlclose(handle);
        return -1;
    }


    typedef int (*load_change_policy)(char *, int);

    load_change_policy change_policy =
        reinterpret_cast<load_change_policy>(func);
    if (change_policy == NULL) {
        LOG_E("change_policy err: %s.\n", dlerror());
        dlclose(handle);
        return -1;
    }

    ret = change_policy(conf_name, start);
    if(ret < 0) {/*change poilcy fail*/
        LOG_E("change_policy err: %d, %s\n", ret, conf_name);
        dlclose(handle);
        return -1;
    }

    /*record thermal policies from perf service*/
    ret = thm_load_thm_data();
    if (ret < 0) {
        LOG_E("thm_load_thm_data err: %d.\n", ret);
        dlclose(handle);
        return ret;
    }

    next_index = thm_get_next_conf(conf_name, start);
    if (next_index < 0)
        LOG_D("next_idx err: %d\n", next_index);

    ret = thm_store_thm_data();
    if (ret < 0) {
        LOG_E("thm_store_thm_data err: %d.\n", ret);
        dlclose(handle);
        return ret;
    }

    dlclose(handle);
    return 0;
}

int reset_thermal_policy(int power_on_init)
{
    void *handle, *func;
    int ret = 0;
    //char prop_content[PROPERTY_VALUE_MAX] = "\0";
    //int prop_value = 1;

    LOG_I("");
    policy_status.max_id = -1;
    policy_status.active_policy = -2;
    policy_status.list_len = 0;
    policy_status.list = NULL;

    if(power_on_init == 1) {
        LOG_I("skip");
        return 0;
    }

    handle = dlopen(THM_LIB_FULL_NAME, RTLD_NOW);

    LOG_I("start");

    if (handle == NULL) {
        LOG_E("Can't load library: %s", dlerror());
        return -1;
    }

    /*reset thermal policy counter*/
    func = dlsym(handle, "reset_propertydata");
    if(func == NULL) {
        LOG_E("dlsym: reset_propertydata fail:%s",
        dlerror());
        dlclose(handle);
        return -1;
    }

    typedef int (*reset_thermal_policy)(void);

    reset_thermal_policy reset_propertydata =
        reinterpret_cast<reset_thermal_policy>(func);
    if (reset_propertydata == NULL) {
        LOG_E("reset_propertydata err: %s.\n",
        dlerror());
        dlclose(handle);
        return -1;
    }

    ret = reset_propertydata();
    if(ret < 0) {
        LOG_E("reset_propertydata ret: %d\n", ret);
        dlclose(handle);
        return -1;
    }
    /*reset thermal policy counter*/

   dlclose(handle);
   return 0;

}


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

#include <aidl/vendor/mediatek/hardware/netdagent/INetdagents.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <utils/Log.h>
#include <errno.h>
#include <cutils/properties.h>

#include "utility_netd.h"
#include "perfservice.h"

#include <vendor/mediatek/hardware/netdagent/1.0/INetdagent.h>

/* IP filter */
#define MAX_NETD_IP_FILTER_COUNT   (3)
#define MAX_NETD_IP_FILTER_LEN     (256)

/* blocking feature */
#define MAX_NETD_BLOCK_UID_COUNT   (20)

/* boost feature */
#define MAX_NETD_BOOST_UID_GROUP   (4)  // APK booster
#define MAX_NETD_BOOST_UID_COUNT   (5)

//using namespace std;
using android::hardware::Return;
using android::hardware::hidl_string;
using vendor::mediatek::hardware::netdagent::V1_0::INetdagent;

using ::aidl::vendor::mediatek::hardware::netdagent::INetdagents;
using ::ndk::SpAIBinder;
using ::ndk::ScopedAStatus;

/* IP fliter feature */
static int nIpFilterCurIdx = 0;
static char **pIpFilterTbl = NULL;

/* boost feature */
static int boostUidTbl[MAX_NETD_BOOST_UID_COUNT]; /* PERF_RES_NET_NETD_BOOST_UID == 2 */
static int lastFgBoostPid = -1;

// APK booster @{
/*Group means VIP channel number now*/
static int boostUidTblx[MAX_NETD_BOOST_UID_GROUP][MAX_NETD_BOOST_UID_COUNT];
static int lastFgBoostUidx[MAX_NETD_BOOST_UID_GROUP] = {-1, -1, -1, -1};

static int booster_netd_config_by_uid(int group, bool enable, int uid);
static int booster_netd_config_by_linkinfo(int group, bool enable, const char *data);
static int booster_netd_config_init_auto(int init);
// @}

/**
 * Find AIDL service and execute it if available.
 * @param argv, command string.
 * @return true for success and false for fail.
 */
static bool finishedWithAidl(const char *argv) {
    string cmd;
    bool ret;
    ndk::ScopedAStatus status;

    std::string ins = std::string() + INetdagents::descriptor + "/default";
    std::shared_ptr<INetdagents> netdagent;

    if (argv == NULL) {
        LOG_E("argv is NULL");
        return false;
    }
    cmd = argv;

    if (fprintf(stderr, "%s\n", cmd.c_str()) < 0) {
        LOG_E("fprintf error return");
        return false;
    }

    netdagent = INetdagents::fromBinder(SpAIBinder(AServiceManager_getService(ins.c_str())));
    if (netdagent != nullptr) {
        status = netdagent->dispatchNetdagentCmd(cmd, &ret);
        if (!status.isOk()) {
            LOG_E("aidl:netdagent->dispatchNetdagentCmd() with %s fail\n", cmd.c_str());
        } else {
            LOG_E("aidl:netdagent->dispatchNetdagentCmd() with %s ok, ret %d\n", cmd.c_str(), ret);
        }
        return true;
    } else {
        LOG_I("get %s aidl service failed\n", INetdagents::descriptor);
        return false;
    }
}

/* NETD agent */
static int NetdAgentCmd(const char *argv, int log)
{
    hidl_string hidl_cmd;
    string temp_cmd;

    if (argv == NULL) {
        LOG_E("argv is NULL");
        return -1;
    }

    if (finishedWithAidl(argv)) {
        return 0;
    }

    temp_cmd = argv;
    temp_cmd += " ";

    if (log)
        LOG_D("%s", temp_cmd.c_str());

    android::sp<INetdagent> gNetdagentService;
    //get Netdagent HIDL service
    gNetdagentService = INetdagent::tryGetService();
    if (gNetdagentService == nullptr) {
        LOG_E("get %s service failed", INetdagent::descriptor);
        return -1;
    }

    //execute  netdagent
    hidl_cmd = temp_cmd;
    Return<bool> ret = gNetdagentService->dispatchNetdagentCmd(hidl_cmd);
    if(!ret.isOk()) {
        LOG_E("dispatchNetdagentCmd is not ok");
        return -1;
    }

    if(ret == false) {
        LOG_E("dispatchNetdagentCmd failed");
        return -1;
    }
    return 0;
}

extern int netd_reset(int power_on_init)
{
    int ret = 0, i;
    char cmd[256];

    // APK booster @{
    booster_netd_config_init_auto(power_on_init);
    // @}

    if(power_on_init != 1) {
        memset(cmd, 0, 256) ;
        if(sprintf(cmd,"netdagent firewall priority_clear_uid_all") < 0) {
            LOG_E("sprintf fail") ;
            return -1 ;
        }

        if (NetdAgentCmd(cmd, 1) < 0) {
            LOG_E("clear uid fail") ;
            ret = -1;
        }

        memset(cmd, 0, 256) ;
        if(sprintf(cmd,"netdagent firewall priority_clear_toup_all") < 0) {
            LOG_E("sprintf fail") ;
            return -1 ;
        }

        if (NetdAgentCmd(cmd, 1) < 0) {
            LOG_E("clear toup fail") ;
            ret = -1;
        }
    }

    /* netd ip filter table */
    if((pIpFilterTbl = (char**)malloc(sizeof(char*) * MAX_NETD_IP_FILTER_COUNT)) == NULL) {
        LOG_E("Can't allocate memory");
        return 0;
    }
    for (i=0; i < MAX_NETD_IP_FILTER_COUNT; i++) {
        if((pIpFilterTbl[i] = (char*)malloc(sizeof(char) * MAX_NETD_IP_FILTER_LEN)) == NULL) {
            LOG_E("Can't allocate memory");
            return 0;
        }
        strncpy(pIpFilterTbl[i], "NULL", MAX_NETD_IP_FILTER_LEN-1);
    }
    nIpFilterCurIdx = 0;

    /* boost feature */
    for (i=0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
        boostUidTbl[i] = -1;
    }
    return ret;
}

static void netd_dump_priority_uid(void)
{
    int i;
    for (i=0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
        if (boostUidTbl[i] != -1) {
            LOG_I("Set packet Priority oneshot UID(%d)", boostUidTbl[i]);
        }
    }
}

extern "C"
int netd_set_priority_uid(int uid, void *scn)
{
    char cmd[128];
    memset(cmd ,0,128) ;
    char value[PROPERTY_VALUE_MAX] = "\0";
    int prop_value = 1, i, empty_idx = -1;

    property_get("persist.vendor.powerhal.PERF_RES_NET_NETD_BOOST_UID", value, "1");
    prop_value = atoi(value);
    LOG_D("persist.vendor.powerhal.PERF_RES_NET_NETD_BOOST_UID:%d", prop_value);

    LOG_V("%p", scn);
    netd_dump_priority_uid();

    if (uid == 1) { /* boost foreground */
        getForegroundInfo(NULL, NULL, &uid);
        lastFgBoostPid = uid;
    } else if(uid == 2) {/* always boost */
        getForegroundInfo(NULL, NULL, &uid);

        for (i=0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
            if (boostUidTbl[i] == uid) {
                LOG_I("Set packet Priority UID(%d) before", uid);
                return 0; // already boost
            } else if (boostUidTbl[i] == -1 && empty_idx == -1) {
                empty_idx = i; // find empty slot
            }
        }

        if (empty_idx == -1) {
            LOG_E("Set packet Priority UID(%d) fail: no more empty slot", uid);
            return -1;
        }
    }

    // return directly if prop is set to 0
    if (prop_value == 0)
        return 0;

    if(sprintf(cmd,"netdagent firewall priority_set_uid %d",uid) < 0) {
        LOG_E("sprintf fail") ;
        return -1 ;
    }

    if (NetdAgentCmd(cmd, 1) < 0) {
        LOG_E("SetPriorityWithUID fail") ;
        return -1 ;
    }

    if (empty_idx != -1) /* input uid == 2 */
        boostUidTbl[empty_idx] = uid;

    LOG_I("Set packet Priority UID(%d)", uid);
    return 0;
}

extern "C"
int netd_clear_priority_uid(int uid, void *scn)
{
    char cmd[128];
    memset(cmd ,0,128) ;

    LOG_V("%p", scn);
    if (uid == 2) {
        LOG_I("Clear packet Priority UID(%d) skip", uid) ;
        return 0;
    }

    if (uid == 1 && lastFgBoostPid != -1) {
        uid = lastFgBoostPid;
        lastFgBoostPid = -1;
    }

    if(sprintf(cmd,"netdagent firewall priority_clear_uid %d" ,uid) < 0) {
        LOG_E("sprintf fail") ;
        return -1 ;
    }

    if (NetdAgentCmd(cmd, 1) < 0){
        LOG_E("ClearPriorityWithUID fail") ;
        return -1 ;
    }
    LOG_I("Clear packet Priority UID(%d)", uid) ;
    return 0;
}

extern "C"
int netd_set_block_uid(int uid, void *scn)
{
    static int block_uid[MAX_NETD_BLOCK_UID_COUNT];
    static int block_uid_init = -1;
    int i, empty_idx = -1;
    char cmd[128];
    memset(cmd ,0,128);

    LOG_D("%d,%p", uid, scn);

    if (block_uid_init == -1) {
        LOG_I("init");
        for (i=0; i<MAX_NETD_BLOCK_UID_COUNT; i++)
            block_uid[i] = -1;
        block_uid_init = 1;
    }

    if(uid == 1) {
        getForegroundInfo(NULL, NULL, &uid);
    }

    /* check is uid blocked before ? */
    for (i=0; i<MAX_NETD_BLOCK_UID_COUNT; i++) {
        //LOG_I("block_uid[%d]=%d", i, block_uid[i]);
        if (block_uid[i] == uid) { // blocked before
            LOG_I("already blocked(%d)", uid);
            return 0;
        }
        else if (block_uid[i] == -1 && empty_idx == -1) {
            empty_idx = i; // first empty
        }
    }

    //LOG_I("empty_idx:%d", empty_idx);
    if (empty_idx == -1) {
        LOG_E("set_blocking_enable fail (no more empty slot)") ;
        return -1;
    }

    if(sprintf(cmd,"netdagent firewall set_blocking_enable %d",uid) < 0) {
        LOG_E("sprintf fail") ;
        return -1 ;
    }

    if (NetdAgentCmd(cmd, 1) < 0) {
        LOG_E("set_blocking_enable fail") ;
        return -1 ;
    }
    LOG_I("set_blocking_enable(%d)", uid);

    /* save block uid */
    if (empty_idx != -1) {
        block_uid[empty_idx] = uid;
        //LOG_I("block_uid[%d]=%d", empty_idx, block_uid[empty_idx]);
    }

    return 0;
}

extern "C"
int netd_unset_block_uid(int uid, void *scn)
{
    LOG_D("%d,%p", uid, scn);
    return 0;
}

static int SetDupPacketLink(const char *linkInfo)
{
    char cmd[256];
    memset(cmd, 0, 256) ;
    if (snprintf(cmd, sizeof(cmd), "netdagent firewall priority_set_toup %s", linkInfo) < 0) {
        LOG_E("sprintf fail");
        return -1;
    }

    if (NetdAgentCmd(cmd, 0) < 0) {
        LOG_E("SetDupPacketLink fail") ;
        return -1 ;
    }
    //LOG_D("SetDupPacketLink(%s)",linkInfo) ;
    LOG_D("");

    return 0;
}

static int ClearDupPacketLink(const char *linkInfo)
{
    char cmd[256];
    memset(cmd, 0, 256) ;
    if(sprintf(cmd,"netdagent firewall priority_clear_toup %s",linkInfo) < 0) {
        LOG_E("sprintf fail") ;
        return -1 ;
    }

    if (NetdAgentCmd(cmd, 0) < 0) {
        LOG_E("fail") ;
        return -1 ;
    }
    //LOG_D("(%s)",linkInfo) ;
    LOG_D("");

    return 0;
}

int deleteAllDupPackerLink(void)
{
    int i;

    for(i=0; i<MAX_NETD_IP_FILTER_COUNT; i++) {
        if(strcmp(pIpFilterTbl[i], "NULL") != 0) {
            if(ClearDupPacketLink(pIpFilterTbl[i]) < 0) {
                return -1;
            }
            strncpy(pIpFilterTbl[i], "NULL", MAX_NETD_IP_FILTER_LEN-1);
        }
    }
    nIpFilterCurIdx = 0;
    return 0;
}

int SetOnePacketLink(const char *linkInfo)
{
    if(strcmp(pIpFilterTbl[nIpFilterCurIdx], "NULL") != 0) {
        ClearDupPacketLink(pIpFilterTbl[nIpFilterCurIdx]);
    }

    if(SetDupPacketLink(linkInfo) < 0) {
        return -1;
    }

    strncpy(pIpFilterTbl[nIpFilterCurIdx], linkInfo, MAX_NETD_IP_FILTER_LEN-1);
    nIpFilterCurIdx = (nIpFilterCurIdx+1) % MAX_NETD_IP_FILTER_COUNT;
    return 0;
}

int SetDupPacketMultiLink(const char *linkInfo)
{
    char linkStr[256];
    char *str = NULL, *saveptr = NULL;
    int i, count = 0;

    /* always delete all first */
    if(deleteAllDupPackerLink() < 0)
        return -1;

    /* format: MULTI count IP_pair_1 IP_pair_2 IP_pair_3 */
    strncpy(linkStr, linkInfo, sizeof(linkStr)-1);
    linkStr[255] = '\0';
    str = strtok_r(linkStr, "\t", &saveptr); /* "MULTI"  */
    if (saveptr == NULL) {
        LOG_E("saveptr is NULL");
        return -1;
    }
    str = strtok_r(NULL, "\t", &saveptr);    /* count    */
    count = atoi(str);

    /* check count */
    if(count > MAX_NETD_IP_FILTER_COUNT)
        return -1;

    /* set all ip pair */
    for(i=0; i<count; i++) {
        str = strtok_r(NULL, "\t", &saveptr);
        if(SetDupPacketLink(str) < 0) {
            return -1;
        }
        strncpy(pIpFilterTbl[nIpFilterCurIdx], str, MAX_NETD_IP_FILTER_LEN-1);
        nIpFilterCurIdx = (nIpFilterCurIdx+1) % MAX_NETD_IP_FILTER_COUNT;
    }
    return 0;
}
/*******************************SDK API **************************/
/**
 * clear all the vip channel rules, include pid rules and 5-tuples rules
 *  @type:which type rules will be cleared
*/
#define RULE_TYPE_UID 1
#define RULE_TYPE_LINKINFO 2
#define RULE_TYPE_ALL 3

int sdk_netd_clearall(int type)
{
	int ret = 0;
	char cmd[256];

	if (type == RULE_TYPE_UID || type == RULE_TYPE_ALL) {
		memset(cmd, 0, 256) ;
		if(sprintf(cmd,"netdagent firewall priority_clear_uid_all") < 0) {
			LOG_E("sprintf fail");
			return -1;
		}

		if (NetdAgentCmd(cmd, 1) < 0) {
			LOG_E("clear uid fail");
			ret = -1;
		}
	}

	if (type == RULE_TYPE_LINKINFO || type == RULE_TYPE_ALL) {
		memset(cmd, 0, 256) ;
		if(sprintf(cmd,"netdagent firewall priority_clear_toup_all") < 0) {
			LOG_E("sprintf fail");
			return -1;
		}

		if (NetdAgentCmd(cmd, 1) < 0) {
			LOG_E("clear toup fail");
			ret = -1;
		}
	}
	return ret;
}

/**
 *  set packets of APK to go through high priority HW queue
 */
int sdk_netd_set_priority_by_uid(int uid)
{
	char cmd[128];
	memset(cmd ,0,128);

  	if (uid <0)
		return -1;
	if(sprintf(cmd,"netdagent firewall priority_set_uid %d",uid) < 0) {
		LOG_E("sprintf fail");
		return -1;
	}

	if (NetdAgentCmd(cmd, 1) < 0) {
		LOG_E("SetPriorityWithUID fail");
		return -1;
	}
	LOG_I("Set packet Priority UID(%d)", uid);
	return 0;
}
/**
 *  clear high priority rules set by uid
 */
int sdk_netd_clear_priority_by_uid(int uid)
{
	char cmd[128];
	memset(cmd ,0,128);

  	if (uid <0)
		return -1;
  
	if(sprintf(cmd,"netdagent firewall priority_clear_uid %d" ,uid) < 0) {
		LOG_E("sprintf fail");
		return -1;
	}

	if (NetdAgentCmd(cmd, 1) < 0){
		LOG_E("ClearPriorityWithUID fail");
		return -1;
	}
	LOG_I("Clear packet Priority UID(%d)", uid);
	return 0;
}
static inline const char * sdk_set_linkinof(const char *paremeter) {

	if (!paremeter)
		return "none";
	else
		return paremeter;
}
/**
 *  set packets of APK to go through high priority HW queue by 5-tuples
 *  @src_ip/dst_ip: source ip / desination ip ,if you don't want to match the paremeters, set them to NULL
 *  @src_port/dst_port: source port or desination port ,the two input paremeters are invalide if you don't set protocol to UDP or TCP
 *   if you don't want to match the paremeters, set them to NULL
 *  @protocol: if you don't want to match the paremeter, set it to NULL
 */
int sdk_netd_set_priority_by_linkinfo(const char *src_ip,  const char *src_port, const char *dst_ip,
                                       const char *dst_port, const char *protocol)
{
	char cmd[256];
	memset(cmd, 0, 256);
	if(sprintf(cmd,"netdagent firewall priority_set_toup %s %s %s %s %s",sdk_set_linkinof(src_ip),
		sdk_set_linkinof(src_port), sdk_set_linkinof(dst_ip), sdk_set_linkinof(dst_port), sdk_set_linkinof(protocol)) < 0) {
		LOG_E("sdk_netd_set_priority_by_linkinfo sprintf fail");
		return -1;
	}

	if (NetdAgentCmd(cmd, 0) < 0) {
		LOG_E("sdk_netd_set_priority_by_linkinfo NetdAgentCmd fail");
		return -1;
	}
	LOG_D("Set packet Priority by linkinfo(%s)", cmd);
	return 0;
}

/**
 *  clear packets of APK to go through high priority HW queue by 5-tuples
 *  @src_ip/dst_ip: source ip / desination ip ,if you don't want to match the paremeters, set them to NULL
 *  @src_port/dst_port: source port or desination port ,the two input paremeters are invalide if you don't set protocol to UDP or TCP
 *   if you don't want to match the paremeters, set them to NULL
 *  @protocol: if you don't want to match the paremeter, set it to NULL
 */
int sdk_netd_clear_priority_by_linkinfo(const char *src_ip,  const char *src_port, const char *dst_ip,
                                       const char *dst_port, const char *protocol)
{
	char cmd[256];
	memset(cmd, 0, 256);

	if(sprintf(cmd,"netdagent firewall priority_clear_toup %s %s %s %s %s",sdk_set_linkinof(src_ip),
		sdk_set_linkinof(src_port), sdk_set_linkinof(dst_ip), sdk_set_linkinof(dst_port), sdk_set_linkinof(protocol)) < 0) {
		LOG_E("sdk_netd_clear_priority_by_linkinfo sprintf fail");
		return -1;
	}

	if (NetdAgentCmd(cmd, 0) < 0) {
		LOG_E("sdk_netd_clear_priority_by_linkinfo NetdAgentCmd fail");
		return -1;
	}
	LOG_D("clear packet Priority by linkinfo(%s)", cmd);
	return 0;
}

// APK Booster @{
/**
 * Dump always enable booster rules information.
 */
void netd_dump_priority_uid_ext(void) {
    int i;
    int g;
    for (g = 0; g < MAX_NETD_BOOST_UID_GROUP; g++) {
        for (i = 0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
            if (boostUidTblx[g][i] != -1) {
                LOG_I("[Booster]: %s, boostUidTblx[%d][%d]: %d", __func__, g, i, boostUidTblx[g][i]);
            }
        }
    }
}

/**
 * Called by configuring powerhal xml to process auto UID related request.
 * this design maybe get fail because the first parameter uid > 2 is a real uid?
 * @param gact group * 100 + action. they are command group and command action.
 *        PS, power hal only can config param1 (gact) decimal int string.
 *        So, the max is 2,147,483,647. at present, (0 ~ 99) * 10000 for backward
 *        compatible with old feature 1 froeground and 2 always. (0 ~ 99) * 100 for
 *        group and 0 ~ 99 for action.
 * @param scn unused now.
 * @return 0 for success and -1 for fail.
 */
// extern "C" int booster_netd_process_auto_uid_request(int gact, void *scn) {

/**
 * Called by configuring powerhal xml to process auto UID set request.
 * @param uid parameter from power hal
 * @param scn unused now.
 * @return 0 for success and -1 for fail.
 */
extern "C" int netd_set_priority_uid_2(int uid, void *scn) {
    const int group = 2;  // this function for auto mode is only for VIP channel 2
    char cmd[128];
    char value[PROPERTY_VALUE_MAX] = "\0";
    int pvalue = 1, i = 0, emptyidx = -1, ret = -1;
    memset(cmd, 0, 128);
    LOG_I("[Booster]: %s, uid: %d", __func__, uid);

    property_get("persist.vendor.powerhal.PERF_RES_NET_NETD_BOOST_UID", value, "1");
    pvalue = atoi(value);
    LOG_D("[Booster]: %s, PERF_RES_NET_NETD_BOOST_UID: %d, uid: %d", __func__, pvalue, uid);

    LOG_V("%p", scn);
    netd_dump_priority_uid_ext();

    if (uid == 1) { /* boost foreground */
        getForegroundInfo(NULL, NULL, &uid);
        LOG_I("[Booster]: %s, group: %d, uid: %d strongly recommand to cofigure as 2", __func__,
                group, uid);
        if (lastFgBoostUidx[group - 1] == uid) {
            // this is auto mode for this case, it must be duplicate rule addition, so return -1.
            LOG_E("[Booster]: %s, group: %d, uid: %d dup added", __func__, group, uid);
            return -1;
        }
        lastFgBoostUidx[group - 1] = uid;
    } else if (uid == 2) {/* always boost */
        getForegroundInfo(NULL, NULL, &uid);
        LOG_I("[Booster]: %s, uid: %d", __func__, uid);
        for (i = 0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
            if (boostUidTblx[group - 1][i] == uid) {
                LOG_D("[Booster]: %s, group: %d, uid: %d existed", __func__, group, uid);
                return 0;  // already boost
            } else if (boostUidTblx[group - 1][i] == -1 && emptyidx == -1) {
                emptyidx = i;  // find empty slot
            }
        }

        if (emptyidx == -1) {
            LOG_E("[Booster]: %s, uid: %d boost fail, no more empty slot", __func__, uid);
            return -1;
        }
    }

    // return directly if prop is set to 0
    if (pvalue == 0) {
        LOG_E("[Booster]: %s, uid: %d, group: %d, pvalue = 0", __func__, uid, group);
        return 0;
    }

    ret = booster_netd_config_by_uid(group, true, uid);

    if (emptyidx != -1 && !ret) {/* input uid == 2 */
        boostUidTblx[group - 1][emptyidx] = uid;
    }

    return ret;
}

/**
 * Called by configuring powerhal xml to process auto UID clear request.
 * @param uid parameter from power hal
 * @param scn unused now.
 * @return 0 for success and -1 for fail.
 */
extern "C" int netd_clear_priority_uid_2(int uid, void *scn) {
    const int group = 2;  // this function for auto mode is only for VIP channel 2
    char cmd[128];
    int ret = -1;
    memset(cmd, 0, 128);
    LOG_I("[Booster]: %s, uid: %d", __func__, uid);

    LOG_V("%p", scn);
    if (uid == 2) {
        LOG_D("[Booster]: %s, uid: %d skip", __func__, uid);
        return 0;
    }

    if (uid == 1 && lastFgBoostUidx[group - 1] != -1) {
        uid = lastFgBoostUidx[group - 1];
        lastFgBoostUidx[group - 1] = -1;
    }

    ret = booster_netd_config_by_uid(group, false, uid);

    return ret;
}

/**
 * Called by netd_reset to process auto UID init request.
 * netd_reset is called by configuring powerhal xml.
 * seems called once only at bootup target.
 * @param init from power hal.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_config_init_auto(int init) {
    int i = 0, g = 0;
    LOG_I("[Booster]: %s, init: %d", __func__, init);

    if (init != 1) {
        for (g = 1; g <= MAX_NETD_BOOST_UID_GROUP; g++) {
            booster_netd_config_by_uid(g, false, -1);
            booster_netd_config_by_linkinfo(g, false, NULL);
        }
    }

    for (g = 0; g < MAX_NETD_BOOST_UID_GROUP; g++) {
        for (i = 0; i < MAX_NETD_BOOST_UID_COUNT; i++) {
            boostUidTblx[g][i] = -1;
        }
    }
    return 0;
}

/**
 * Configure or remove booster rule by UID.
 * @param group, command group.
 * @param enable, true for add and false for remove when uid > -1.
 *                invalid for uid == -1.
 * @param uid, uid < -1 is invalid parameter, uid == -1 indicates remove all uid rules for
 *             specified group, uid > -1 is a valid parameter to configure.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_config_by_uid(int group, bool enable, int uid) {
    char cmd[256];
    char tcmd[64];
    char ustr[15];
    char *set = (char *)"priority_set_uid";
    char *clr = (char *)"priority_clear_uid";
    char *clrall = (char *)"priority_clear_uid_all";
    char *optr = NULL;
    memset(cmd, 0, 256);
    memset(tcmd, 0, 64);
    memset(ustr, 0, 15);
    LOG_I("[Booster]: %s, group: %d, en: %d, uid: %d", __func__, group, enable, uid);

    if (enable) {
        optr = set;
    } else {
        optr = clr;
    }

    if (uid < -1) {
        LOG_E("[Booster]: %s, uid < -1: %d", __func__, uid);
        return -1;
    } else if (uid == -1) {
        optr = clrall;
        if (snprintf(ustr, sizeof(ustr), "%s", "") < 0) {
            LOG_E("[Booster]: %s, snprintf ustr: \"\" uid: %d fail", __func__, uid);
            return -1;
        }
    } else {
        if (snprintf(ustr, sizeof(ustr), "%d", uid) < 0) {
            LOG_E("[Booster]: %s, snprintf uid: %d fail", __func__, uid);
            return -1;
        }
    }

    // duplicate code, optimize it later.
    if (group < 1) {
        LOG_E("[Booster]: %s, group < 1: %d", __func__, group);
        return -1;
    } else {
        if (snprintf(tcmd, sizeof(tcmd), "%s.%d", optr, group) < 0) {
            LOG_E("[Booster]: %s, snprintf optr: %s group: %d fail", __func__, optr, group);
            return -1;
        }
    }

    if (snprintf(cmd, sizeof(cmd), "netdagent firewall %s %s", tcmd, ustr) < 0) {
        LOG_E("[Booster]: %s, snprintf cmd: %s, ustr: %s fail", __func__, cmd, ustr);
        return -1;
    }

    if (NetdAgentCmd(cmd, 1) < 0) {
        LOG_E("[Booster]: %s fail, cmd: %s", __func__, cmd);
        return -1;
    }
    LOG_D("[Booster]: %s success, cmd: %s", __func__, cmd);
    return 0;
}

/**
 * Configure or remove booster rule by linkinfo.
 * @param group, command group.
 * @param enable, true for configure and false for remove when data isn't null.
 *                invalid when data is null.
 * @param data, 5-tuple infomation for specified set and clear, null is to remove all.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_config_by_linkinfo(int group, bool enable, const char *data) {
    char cmd[256];
    char tcmd[64];
    char lstr[192];
    char buf[192];
    char *set = (char *)"priority_set_toup";
    char *clr = (char *)"priority_clear_toup";
    char *clrall = (char *)"priority_clear_toup_all";
    const char *delim = " ";
    char *optr = NULL, *saveptr = NULL;
    memset(cmd, 0, 256);
    memset(tcmd, 0, 64);
    memset(lstr, 0, 192);
    memset(buf, 0, 192);
    LOG_I("[Booster]: %s, group: %d, en: %d, data: %s", __func__, group, enable, data);

    if (enable) {
        optr = set;
    } else {
        optr = clr;
    }

    if (!data) {
        optr = clrall;
        if (snprintf(lstr, sizeof(lstr), "%s", "") < 0) {
            LOG_E("[Booster]: %s, snprintf lstr: \"\" data: %s fail", __func__, data);
            return -1;
        }
    } else {
    /**
     * set packets of APK to go through high priority HW queue by 5-tuples.
     * @srcip/dstip: source ip / desination ip ,if you don't want to match the paremeters,
     * set them to NULL
     * @srcport/dstport: source port / desination port, the two input paremeters are invalide
     * if you don't set protocol to UDP or TCP. if you don't want to match the paremeters,
     * set them to NULL
     * @proto: if you don't want to match the paremeter, set it to NULL
     */
        strncpy(buf, data, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *srcip = strtok_r(buf, delim, &saveptr);
        if (!srcip || !saveptr) {
            LOG_E("[Booster]: %s, error srcip: %.4s, saveptr: %s", __func__, srcip, saveptr);
            return -1;
        }
        char *srcport = strtok_r(NULL, delim, &saveptr);
        if (!srcport || !saveptr) {
            LOG_E("[Booster]: %s, error srcport: %s, saveptr: %s", __func__, srcport, saveptr);
            return -1;
        }
        char *dstip = strtok_r(NULL, delim, &saveptr);
        if (!dstip || !saveptr) {
            LOG_E("[Booster]: %s, error dstip: %.4s, saveptr: %s", __func__, dstip, saveptr);
            return -1;
        }
        char *dstport = strtok_r(NULL, delim, &saveptr);
        if (!dstport || !saveptr) {
            LOG_E("[Booster]: %s, error dstport: %s, saveptr: %s", __func__, dstport, saveptr);
            return -1;
        }
        char *proto = strtok_r(NULL, delim, &saveptr);
        if (!proto) {
            LOG_E("[Booster]: %s, error proto: %s, saveptr: %s", __func__, proto, saveptr);
            return -1;
        }

        if (snprintf(lstr, sizeof(lstr), "%s %s %s %s %s", sdk_set_linkinof(srcip),
                sdk_set_linkinof(srcport), sdk_set_linkinof(dstip), sdk_set_linkinof(dstport),
                sdk_set_linkinof(proto)) < 0) {
            LOG_E("[Booster]: %s, snprintf srcip: %s:%s dstip: %s:%s proto: %s fail", __func__,
                    srcip, srcport, dstip, dstport, proto);
            return -1;
        }
    }

    // duplicate code, optimize it later.
    if (group < 1) {
        LOG_E("[Booster]: %s, group < 1: %d", __func__, group);
        return -1;
    } else {
        if (snprintf(tcmd, sizeof(tcmd), "%s.%d", optr, group) < 0) {
            LOG_E("[Booster]: %s, snprintf optr: %s group: %d fail", __func__, optr, group);
            return -1;
        }
    }

    if (snprintf(cmd, sizeof(cmd), "netdagent firewall %s %s", tcmd, lstr) < 0) {
        LOG_E("[Booster]: %s, snprintf cmd: %s, lstr: %s fail", __func__, cmd, lstr);
        return -1;
    }

    if (NetdAgentCmd(cmd, 1) < 0) {
        LOG_E("[Booster]: %s fail, cmd: %s", __func__, cmd);
        return -1;
    }
    LOG_D("[Booster]: %s success, cmd: %s", __func__, cmd);
    return 0;
}

/**
 * Process UID related request.
 * @param act, command to process UID.
 * @param data, UID infomation.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_process_uid_request(const char *act, const char *data) {
    /* store data */
    char buf[256];
    const char *delim = " ";
    char *token = NULL, *saveptr = NULL;

    int ret = -1;
    int uid = -1;
    LOG_I("[Booster]: %s, act: %s, data: %s", __func__, act, data);

    if (!act) {
        LOG_E("[Booster]: %s, error act: %s", __func__, act);
        return -1;
    }

    if (data) {
        memset(buf, 0, 256);
        strncpy(buf, data, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        token = strtok_r(buf, delim, &saveptr);

        if (token != NULL) {
            uid = atoi(token);
            if (uid < 0) {
                LOG_E("[Booster]: %s, error uid: %d for act: %s", __func__, uid, act);
                return -1;
            }
            // UIDSET_x x is group concept, group 1 can replace old APIs for VIP channel 1.
            if (!strncmp(act, "UIDSET_1", 8)) {
                ret = booster_netd_config_by_uid(1, true, uid);
            } else if (!strncmp(act, "UIDSET_2", 8)) {
                ret = booster_netd_config_by_uid(2, true, uid);
            } else if (!strncmp(act, "UIDSET_3", 8)) {
                ret = booster_netd_config_by_uid(3, true, uid);
            } else if (!strncmp(act, "UIDSET_4", 8)) {
                ret = booster_netd_config_by_uid(4, true, uid);
            } else if (!strncmp(act, "UIDCLEAR_1", 10)) {
                ret = booster_netd_config_by_uid(1, false, uid);
            } else if (!strncmp(act, "UIDCLEAR_2", 10)) {
                ret = booster_netd_config_by_uid(2, false, uid);
            } else if (!strncmp(act, "UIDCLEAR_3", 10)) {
                ret = booster_netd_config_by_uid(3, false, uid);
            } else if (!strncmp(act, "UIDCLEAR_4", 10)) {
                ret = booster_netd_config_by_uid(4, false, uid);
            } else {
                LOG_E("[Booster]: %s, unsupported UID act: %s", __func__, act);
                return -1;
            }
        } else {
            LOG_E("[Booster]: %s, unknown UID act: %s, data: %s", __func__, act, data);
            return -1;
        }
    } else {
        /**
         * char s[30] = "UIDCLEARALL_1"
         * char *str = s, *saveptr = NULL;
         * char *token = strtok_r(str, " ", &saveptr);
         * for this case, the strtok_r in Android will return NULL for saveptr
         * but for c++ source code, strtok_r will give "\0" address to saveptr, it is not NULL.
         */
        if (!strncmp(act, "UIDCLEARALL_1", 13)) {
            ret = booster_netd_config_by_uid(1, false, -1);
        } else if (!strncmp(act, "UIDCLEARALL_2", 13)) {
            ret = booster_netd_config_by_uid(2, false, -1);
        } else if (!strncmp(act, "UIDCLEARALL_3", 13)) {
            ret = booster_netd_config_by_uid(3, false, -1);
        } else if (!strncmp(act, "UIDCLEARALL_4", 13)) {
            ret = booster_netd_config_by_uid(4, false, -1);
        } else {
            LOG_E("[Booster]: %s, unsupported UID remove act: %s", __func__, act);
            return -1;
        }
    }

    return ret;
}

/**
 * Process link info related request.
 * @param act, command to process link info.
 * @param data, link infomation.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_process_linkinfo_request(const char *act, const char *data) {
    int ret = -1;
    LOG_I("[Booster]: %s, act: %s, data: %s", __func__, act, data);

    if (!act) {
        LOG_E("[Booster]: %s, error act: %s", __func__, act);
        return -1;
    }

    if (!strncmp(act, "LINFOSET_1", 10)) {
        ret = booster_netd_config_by_linkinfo(1, true, data);
    } else if (!strncmp(act, "LINFOSET_2", 10)) {
        ret = booster_netd_config_by_linkinfo(2, true, data);
    } else if (!strncmp(act, "LINFOSET_3", 10)) {
        ret = booster_netd_config_by_linkinfo(3, true, data);
    } else if (!strncmp(act, "LINFOSET_4", 10)) {
        ret = booster_netd_config_by_linkinfo(4, true, data);
    } else if (!strncmp(act, "LINFOCLEAR_1", 12)) {
        ret = booster_netd_config_by_linkinfo(1, false, data);
    } else if (!strncmp(act, "LINFOCLEAR_2", 12)) {
        ret = booster_netd_config_by_linkinfo(2, false, data);
    } else if (!strncmp(act, "LINFOCLEAR_3", 12)) {
        ret = booster_netd_config_by_linkinfo(3, false, data);
    } else if (!strncmp(act, "LINFOCLEAR_4", 12)) {
        ret = booster_netd_config_by_linkinfo(4, false, data);
    } else if (!strncmp(act, "LINFOCLEARALL_1", 15)) {
        ret = booster_netd_config_by_linkinfo(1, false, NULL);
    } else if (!strncmp(act, "LINFOCLEARALL_2", 15)) {
        ret = booster_netd_config_by_linkinfo(2, false, NULL);
    } else if (!strncmp(act, "LINFOCLEARALL_3", 15)) {
        ret = booster_netd_config_by_linkinfo(3, false, NULL);
    } else if (!strncmp(act, "LINFOCLEARALL_4", 15)) {
        ret = booster_netd_config_by_linkinfo(4, false, NULL);
    } else {
        LOG_E("[Booster]: %s, unsupported linkinfo act: %s", __func__, act);
        return -1;
    }

    return ret;
}

/**
 * Process mixed uid / link info request.
 * @param act, command to process uid / link info.
 * @param data, link infomation.
 * @return 0 for success and -1 for fail.
 */
static int booster_netd_process_mixed_request(const char *act, const char *data) {
    int ret = -1;
    LOG_E("[Booster]: %s, for test, act: %s, data: %s", __func__, act, data);

    if (!act) {
        LOG_E("[Booster]: %s, error act: %s", __func__, act);
        return -1;
    }

    if (!strncmp(act, "REMOVEALL_1", 11)) {
        ret = booster_netd_config_by_uid(1, false, -1);
        ret += booster_netd_config_by_linkinfo(1, false, NULL);
    } else if (!strncmp(act, "REMOVEALL_2", 11)) {
        ret = booster_netd_config_by_uid(2, false, -1);
        ret += booster_netd_config_by_linkinfo(2, false, NULL);
    } else if (!strncmp(act, "REMOVEALL_3", 11)) {
        ret = booster_netd_config_by_uid(3, false, -1);
        ret += booster_netd_config_by_linkinfo(3, false, NULL);
    } else if (!strncmp(act, "REMOVEALL_4", 11)) {
        ret = booster_netd_config_by_uid(4, false, -1);
        ret += booster_netd_config_by_linkinfo(4, false, NULL);
    } else {
        LOG_E("[Booster]: %s, unsupported mixed act: %s", __func__, act);
        return -1;
    }

    return ret < 0 ? -1 : 0;
}

/**
 * Receive info from power hal java service and process it by command.
 * @param data, cmd from power hal java service.
 * @return 0 for success and -1 for fail.
 */
int booster_netd_process_request(const char *data) {
    /* store command */
    char buf[256];
    const char *delim = " ";
    char *token = NULL, *saveptr = NULL;
    int ret = -1;

    if (!data) {
        LOG_E("[Booster]: %s, data is NULL", __func__);
        return -1;
    }

    memset(buf, 0, 256);
    strncpy(buf, data, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    token = strtok_r(buf, delim, &saveptr);
    if (token != NULL) {
        if (!strncmp(token, "UID", 3)) {
            ret = booster_netd_process_uid_request(token, saveptr);
        } else if (!strncmp(token, "LINFO", 5)) {
            ret = booster_netd_process_linkinfo_request(token, saveptr);
        } else {
            ret = booster_netd_process_mixed_request(token, saveptr);
        }
    } else {
        LOG_E("[Booster]: strtok_r error, data: %s, buf: %s", data, buf);
        return -1;
    }
    return ret;
}
// @}


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
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/stat.h>

#include "common.h"
#include "utility_mem.h"

#include <sys/socket.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "perfservice.h"
#include "mtkpower_types.h"
//#include <lmkd.h>

#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPerf.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPower.h>
#include <vendor/mediatek/hardware/mtkpower/1.2/IMtkPowerCallback.h>

using namespace vendor::mediatek::hardware::mtkpower::V1_2;
using ::vendor::mediatek::hardware::mtkpower::V1_2::IMtkPowerCallback;

using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
//using namespace std;

/* variable */

static int duraspd_fd = -1;
static int lmkd_fd = -1;
static int originextrafreekbytes = 7777;
static int originswapfreelow = -1;


/***************************************************
final String CPUTHRESHOLD = "cputhreshold";
final String CPUTARGET = "cputarget";
final String POLICYLEVEL = "policylevel";  

*/

static int connect_duraSpeed_socket() {
	int fd = socket_local_client("duraspeed_memory",
				  ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if(fd < 0) {
		ALOGE("[cyliu]Fail to connect to socket duraSpeedMem. return code: %d", fd);
		return -1;
	}
	return fd;
}

/* Entry to trigger duraSpeed */
static void set_duraSpeed(const char *typestr, int val) {
        ALOGI("[cyliu]set cmd = %s , val = %d", typestr, val);
        ALOGI("[cyliu]unsupported!");
/*
	if (duraspd_fd < 0) {
		duraspd_fd = connect_duraSpeed_socket();
		if (duraspd_fd < 0) {
			ALOGE ("[cyliu]set duraSpeed Error, socket connect failed");
			return;
		}
	}

	char buf[256];
	int ret = snprintf(buf, sizeof(buf), "%s:%s:%d\r\n", "iperf", typestr, val);
	if (ret >= (ssize_t)sizeof(buf)) {
		ALOGE ("[cyliu]set duraSpeed Error, out of size");
	}

	ssize_t written;
	written = write(duraspd_fd, buf, strlen(buf) + 1);
	ALOGE ("[cyliu]write duraSpeed (%s)" , buf);
	if (written < 0) {
		ALOGE ("[cyliu]set duraSpeed written:%zu", written);
		close(duraspd_fd);
		duraspd_fd = -1;
		return;
	}
*/
}



extern int invokeScnUpdateCallback(int scn, int data);
/***************************************************

*/
int setLmkSwappiness(int val, void *scn)
{
    string s = to_string(val);
    property_set("vendor.sys.vm.swappiness",(const char *)s.c_str());
    ALOGI("[cyliu][%s]%s val%d", __FILE__, __FUNCTION__, val);

    return 0;
}

/***************************************************

*/
int setLmkWatermarkExtrakbytesadj(int val, void *scn)
{ 
    //int propertyvalue[PROPERTY_VALUE_MAX]={'\0'};
    string setvalstr;

    if(val < 0 || val > 100000){
        ALOGE("[cyliu]unsupport value! val%d", val);
        return 0;
    }
    if (originextrafreekbytes == 7777){ //first init
        //int ret = property_get_int32("vendor.sys.vm.extrafreekbytes", propertyvalue,"0");
        originextrafreekbytes = property_get_int32("vendor.sys.vm.extrafreekbytes", 0);
        ALOGI("[cyliu]%s-1 originextrafreekbytes=%d, errno=%d", __FUNCTION__, originextrafreekbytes, errno);
        //originextrafreekbytes = atoi(propertyvalue);
        //ALOGI("[cyliu]%s-2 originextrafreekbytes=%d", originextrafreekbytes);
    }

    setvalstr = to_string(originextrafreekbytes + val);
    property_set("vendor.sys.vm.extrafreekbytesadj",(const char *)setvalstr.c_str());
    ALOGI("[cyliu]%s-3 org val=%d, adj val=%d  setstr=%s", __FUNCTION__, originextrafreekbytes, val, setvalstr.c_str());

    return 0;
}


/***************************************************

*/
int setLmkWatermarkScalefactor(int val, void *scn)
{ 
    string s = to_string(val);
    property_set("vendor.sys.vm.watermarkscalefactor",(const char *)s.c_str());
    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);

    return 0;
}

/***************************************************

*/
int setLmkDropcaches(int val, void *scn)
{

    string s = to_string(val);
    property_set("vendor.sys.vm.dropcaches",(const char *)s.c_str());
    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);

    return 0;
}

/***************************************************

*/
int setLmkKilltimeout(int val, void *scn)
{

    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);
    string s = to_string(val);
    property_set("vendor.sys.vm.killtimeout",(const char *)s.c_str());
    //invokeScnUpdateCallback(PERF_RES_DRAM_LMK_KILL_TIMEOUT,  val);

    return 0;
}

/***************************************************

*/
int setLmkKillheaviesttask(int val, void *scn)
{

    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);
    invokeScnUpdateCallback(PERF_RES_DRAM_LMK_KILL_HEAVIEST_TASK,  val);

    return 0;
}


/***************************************************

*/
int setLmkReleaseMem(int val, void *scn)
{

    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);
    if(val > 0)
        invokeScnUpdateCallback(PERF_RES_DRAM_LMK_RELEASE_MEM,  val);

    return 0;
}

/***************************************************

*/
int setLmkThrashinglimit(int val, void *scn)
{

    ALOGI("[cyliu][%s]%s val(%d)", __FILE__, __FUNCTION__, val);
    string s = to_string(val);
    property_set("vendor.sys.vm.thrashinglimit",(const char *)s.c_str());
    //invokeScnUpdateCallback(PERF_RES_DRAM_LMK_THRASHING_LIMIT,  val);


    return 0;
}

/**************************************************

*/
int setLmkSwapfreelowpercent(int val, void *scn)
{
    if(val < 0 || val > 100){
        ALOGE("[cyliu]unsupport value! val%d", val);
        return 0;
    }
    ALOGI("[cyliu][%s]%s val%d", __FILE__, __FUNCTION__, val);
    string s = to_string(val);
    property_set("vendor.sys.vm.swaplow",(const char *)s.c_str());
    //invokeScnUpdateCallback(PERF_RES_DRAM_LMK_SWAP_FREE_LOW_PERCENT,  val);


    return 0;
}


int unsetLmkSwapfreelowpercent(int val, void *scn)
{
    string setvalstr;
    //ALOGI("[cyliu][%s]%s val%d", __FILE__, __FUNCTION__, val);


    if (originswapfreelow < 0){ //first init

        originswapfreelow = property_get_int32("ro.lmk.swap_free_low_percentage", 0);
        //ALOGI("[cyliu]%s-1 originswapfreelow=%d, errno=%d", __FUNCTION__, originswapfreelow, errno);
        //originextrafreekbytes = atoi(propertyvalue);
    }


    setvalstr = to_string(originswapfreelow);

    ALOGI("[cyliu]%s-2 originswapfreelow=%d", __FUNCTION__, originswapfreelow);
    property_set("vendor.sys.vm.swaplow",(const char *)setvalstr.c_str());
    //invokeScnUpdateCallback(PERF_RES_DRAM_LMK_SWAP_FREE_LOW_PERCENT,  val);


    return 0;
}
/**************************************************

*/
int setLmkminfreescalefactor(int val, void *scn)
{

    ALOGI("[cyliu][%s]%s val%d",__FILE__ , __FUNCTION__, val);
    invokeScnUpdateCallback(PERF_RES_DRAM_LMK_MINFREE_SCALEFACTOR,  val);


    return 0;
}


/**************************************************

*/
int setDuraspeedCpuThreshold(int val, void *scn)
{
    
    ALOGI("set val%d, scn:%p", val, scn);
    set_duraSpeed("cputhreshold", val);

    return 0;
}

/***************************************************

*/
int setDuraspeedCpuTarget(int val, void *scn)
{

    ALOGI("set val%d, scn:%p", val, scn);
    set_duraSpeed("cputarget", val);


    return 0;
}

/***************************************************

*/
int setDuraspeedPolicylevel(int val, void *scn)
{

    ALOGI("set val%d, scn:%p", val, scn);
    set_duraSpeed("policylevel", val);



    return 0;
}

//SPD: add kswapd0 bind 0~5 core for XLBSSB-2493 by heyuan.li 20230217 start
int kswapd0_bind(int val, void *scn)
{
    property_set("persist.vendor.powerhal.kswapd0_bind_enable", "1");
    ALOGE("kswapd0_bind:%d, scn:%p", val, scn);
    return 0;
}

int kswapd0_unbind(int val, void *scn)
{
    property_set("persist.vendor.powerhal.kswapd0_bind_enable", "0");
    ALOGE("kswapd0_unbind:%d, scn:%p", val, scn);
    return 0;
}
//SPD: add kswapd0 bind 0~5 core for XLBSSB-2493 by heyuan.li 20230217 end





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
#include <cutils/properties.h>

//SPD: add kswapd0 bind 0~5 core for XLBSSB-2493 by heyuan.li 20230217 start
int kswapd0_bind(int val, void *scn)
{
    property_set("persist.odm.tr_powerhal.kswapd0_bind.feature.support", "1");
    ALOGE("kswapd0_bind:%d, scn:%p", val, scn);
    return 0;
}

int kswapd0_unbind(int val, void *scn)
{
    property_set("persist.odm.tr_powerhal.kswapd0_bind.feature.support", "0");
    ALOGE("kswapd0_unbind:%d, scn:%p", val, scn);
    return 0;
}
//SPD: add kswapd0 bind 0~5 core for XLBSSB-2493 by heyuan.li 20230217 end

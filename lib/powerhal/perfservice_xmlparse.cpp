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

#include <utils/Log.h>
#include <utils/RefBase.h>
#include <dlfcn.h>
#include <hidl/HidlSupport.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <expat.h>

#include "common.h"
#include "perfservice.h"
#include "mtkpower_hint.h"
#include "mtkperf_resource.h"
#include "mtkpower_types.h"
#include "perfservice_scn.h"

#include <utils/Timers.h>

#ifdef HAVE_AEE_FEATURE
#include "aee.h"
#endif

#include "tinyxml2.h"
using namespace tinyxml2;

using std::string;
using std::vector;
using android::hardware::hidl_string;

/* Definition */
#define STATE_ON 1
#define STATE_OFF 0

#define PACK_NAME_MAX   128
#define CLASS_NAME_MAX  128

#define APP_LIST_XMLPATH        "/vendor/etc/power_app_cfg.xml"
#define APP_LIST_XMLPATH_2      "/data/vendor/powerhal/power_app_cfg.xml"
#define PACK_LIST_XMLPATH       "/vendor/etc/power_whitelist_cfg.xml"
//#define PACK_LIST_XMLPATH_2     "/data/system/power_whitelist_cfg.xml"
#define PACK_LIST_XMLPATH_2     "/data/vendor/powerhal/power_whitelist_cfg.xml"

#define XMLPARSE_ACTION_MERGE       1
#define XMLPARSE_ACTION_REPLACE     2

#ifdef max
#undef max
#endif
#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a,b) (((a) < (b)) ? (a) : (b))

vector<vector<xml_activity> >  mXmlActList;

static int        mXmlActNum = 0;

static nsecs_t last_aee_time = 0;

const string LESS("less");

tScnConTable tConTable[FIELD_SIZE];

static int check_data_config_validation(void);

static void trigger_fps_aee_warning(const char *aee_log, const char *package, const char *activity)
{
    nsecs_t now = systemTime();
    ALOGD("[perfservice_xmlparse][trigger_aee_warning] pack:%s ack:%s - %s", package, activity, aee_log);

#if defined(HAVE_AEE_FEATURE)
    int interval = ns2ms(now - last_aee_time);
    if (interval > 600000 || interval < 0 || last_aee_time == 0)
        aee_system_warning("powerhal", NULL, DB_OPT_DEFAULT, aee_log);
    else
        ALOGE("[perfservice_xmlparse] trigger_aee_warning skip:%s in %s", aee_log, package);
#endif
    last_aee_time = now;
}

void checkConTable(void) {
    ALOGI("Cmd name, Cmd ID, Entry, default value, current value, compare, max value, min value, isValid, normal value, sport value");
    for(int idx = 0; idx < FIELD_SIZE; idx++) {
        if(tConTable[idx].cmdName.length() == 0)
            continue;
        ALOGI("%s, %d, %s, %d, %d, %s, %d, %d %d %d %d", tConTable[idx].cmdName.c_str(),  tConTable[idx].cmdID, tConTable[idx].entry.c_str(), tConTable[idx].defaultVal,
                tConTable[idx].curVal, tConTable[idx].comp.c_str(), tConTable[idx].maxVal, tConTable[idx].minVal, tConTable[idx].isValid, tConTable[idx].normalVal, tConTable[idx].sportVal);
    }
}

void perfxml_read_cmddata(XMLElement *elmtScenario, int scn)
{
    XMLElement *dataelmt = elmtScenario->FirstChildElement("data");
    int param_1 = 0;
    const char* str = NULL;
    char  cmd[64];

    while(dataelmt){
        str = dataelmt->Attribute("cmd");
        param_1 = dataelmt->IntAttribute("param1");
        ALOGD("[updateCusScnTable] cmd:%s, scn:%d, param_1:%d",
        str, scn, param_1);
        if(str != NULL && strlen(str) < 64)
            set_str_cpy(cmd, str, 64);
        Scn_cmdSetting(cmd, scn, param_1);
        dataelmt = dataelmt->NextSiblingElement();
    }
}

int recount_whitelist_ack_list_num(int action)
{
    int i, j;
    int EraseFlag = 0;

    ALOGD("[count_whitelist_ack_list_num] mXmlActNum:%d size:%lu",  mXmlActNum, (unsigned long)mXmlActList.size());

    for(i = 0; i < mXmlActList.size(); i++) {
        vector<xml_activity>::iterator current = mXmlActList.at(i).begin();
       ALOGD("[count_whitelist_ack_list_num] current pack:%s ack:%s fps:%s window:%s", current->packName, current->actName, current->fps, current->window_mode);

        for (j = i+1; j < mXmlActList.size(); j++) {
            vector<xml_activity>::iterator temp = mXmlActList.at(j).begin();
            ALOGD("[count_whitelist_ack_list_num] temp pack:%s ack:%s fps:%s window:%s", temp->packName, temp->actName, temp->fps, temp->window_mode);
            if (current->delete_flag == 0 &&
                !strcmp(current->packName, temp->packName) && !strcmp(current->actName, temp->actName) && !strcmp(current->fps, temp->fps) && !strcmp(current->window_mode, temp->window_mode)) {
                if (action == XMLPARSE_ACTION_REPLACE) {
                    ALOGI("[count_whitelist_ack_list_num] relace pack:%s ack:%s fps:%s window:%s", current->packName, current->actName, current->fps, current->window_mode);
                    mXmlActList.at(i).clear();
                    mXmlActList.at(i).assign(mXmlActList.at(j).begin(), mXmlActList.at(j).end());
                    current->delete_flag = 1;
                } else {
                    ALOGE("[count_whitelist_ack_list_num] redeclaration of pack:%s, ack:%s", current->packName, current->actName);
                    //ALOGI("[count_whitelist_ack_list_num] merge pack:%s ack:%s", current->packName, current->actName);
                    //vector<xml_activity> row;
                    //row.assign(mXmlActList.at(j).begin(), mXmlActList.at(j).end());
                    //mXmlActList.at(i).reserve(mXmlActList.at(i).size() + row.size());
                    //mXmlActList.at(i).insert(mXmlActList.at(i).end(), row.begin(), row.end());
                    temp->delete_flag = 1;
                }
                EraseFlag = 1;
            }
        }
    }

    if (EraseFlag == 1) {
        vector<vector<xml_activity>>::iterator itm;
        for (itm = mXmlActList.begin(); itm != mXmlActList.end(); ) {
            vector<xml_activity>::iterator its = (*itm).begin();
            if (its->delete_flag == 1) {
                ALOGI("[count_whitelist_ack_list_num] EraseFlag pack:%s ack:%s", its->packName, its->actName);
                while (its != (*itm).end())
                    its = (*itm).erase(its);
                itm = mXmlActList.erase(itm);
            }
            else
                itm++;
        }
    }

    return mXmlActList.size();

}

int update_activity_whitelist_table(const char *path, int action)
{
    XMLDocument docXml;
    //SPD: add xml decode by fan.feng1 20220922 start
    XMLError errXml;
    char *buf_decode = NULL;
    if(tran_powerhal_encode) {
        buf_decode = get_decode_buf(path);
        errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    } else {
        errXml = docXml.LoadFile(path);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    int param_1;

    mXmlActNum = 0;

    ALOGD("[update_activity_whitelist_table]");

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal whitelist config file '%s'. Error: %s",
            __FUNCTION__, path, XMLDocument::ErrorIDToName(errXml));
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    } else {
        ALOGI("%s: load powerhal CusScnTable config succeed!", __FUNCTION__);
    }

    ALOGD("[update_activity_whitelist_table] errXml:%d" , errXml);

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO Pachage info ", __FUNCTION__);
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    }
    XMLElement *elmtPackage = elmtRoot->FirstChildElement("Package");

    while (elmtPackage != NULL) {
        const char* packname = elmtPackage->Attribute("name");
        XMLElement *elmtActivity = elmtPackage->FirstChildElement("Activity");
        while (elmtActivity != NULL) {
            const char* actname = elmtActivity->Attribute("name");
            XMLElement *elmtFPS = elmtActivity->FirstChildElement("FPS");
            if (elmtFPS == NULL) {
                trigger_fps_aee_warning("missing fps mode", packname, actname);
            }

            while (elmtFPS != NULL) {
                const char* FPSvalue = elmtFPS->Attribute("value");
                const char* sbefeaturename = elmtFPS->Attribute("type");
                XMLElement *elmtWINDOW = elmtFPS->FirstChildElement("WINDOW");
                while (elmtWINDOW != NULL) {
                    const char* WINDOWmode = elmtWINDOW->Attribute("mode");
                    XMLElement *dataelmt = elmtWINDOW->FirstChildElement("data");
                    vector<xml_activity> row;
                    if (dataelmt == NULL) {
                        trigger_fps_aee_warning("missing fps data", packname, actname);
                    }
                    while (dataelmt != NULL) {
                        xml_activity temp;
                        const char* cmdname = dataelmt->Attribute("cmd");
                        param_1 = dataelmt->IntAttribute("param1");
                        if(packname != NULL && strlen(packname) < PACK_NAME_MAX)
                            set_str_cpy(temp.packName, packname, PACK_NAME_MAX);
                        if(actname != NULL && strlen(actname) < CLASS_NAME_MAX)
                            set_str_cpy(temp.actName, actname, CLASS_NAME_MAX);
                        if(FPSvalue != NULL && strlen(FPSvalue) < CLASS_NAME_MAX)
                            set_str_cpy(temp.fps, FPSvalue, CLASS_NAME_MAX);
                        if(WINDOWmode != NULL && strlen(WINDOWmode) < CLASS_NAME_MAX)
                            set_str_cpy(temp.window_mode, WINDOWmode, CLASS_NAME_MAX);
                        if(sbefeaturename != NULL && strlen(sbefeaturename) < SBE_NAME_MAX) {
                            set_str_cpy(temp.sbefeatureName, sbefeaturename, SBE_NAME_MAX);
                        } else {
                            set_str_cpy(temp.sbefeatureName, "\0", 1);
                        }
                        if(cmdname != NULL && strlen(cmdname) < 128)
                            set_str_cpy(temp.cmd, cmdname, 128);
                        temp.param1 = param_1;
                        temp.delete_flag = 0;
                        row.push_back(temp);
                        ALOGD("[activity_whitelist table] pack:%s ack:%s fps:%s window:%s type:%s add new cmd:%s param:%d",
                            packname, actname, FPSvalue, WINDOWmode, sbefeaturename, cmdname, param_1);
                        dataelmt = dataelmt->NextSiblingElement();
                    }
                    mXmlActList.push_back(row);
                    mXmlActNum++;

                    elmtWINDOW = elmtWINDOW->NextSiblingElement();
                }

                elmtFPS = elmtFPS->NextSiblingElement();
            }
            elmtActivity = elmtActivity->NextSiblingElement();
        }
        elmtPackage = elmtPackage->NextSiblingElement();
    }
    mXmlActNum = recount_whitelist_ack_list_num(action);
    ALOGI("[update_activity_whitelist_table] total activity num :%d\n", mXmlActNum);

    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode && buf_decode) {
        free(buf_decode);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    return mXmlActNum;
}

void updateScnListfromwhitelist(tScnNode *pPackList)
{
    int i, j, idx;
    int rsc;

    for(i = 0; i < mXmlActList.size(); i++){
        vector<xml_activity>::iterator iter = mXmlActList[i].begin();

        ALOGD("[updateScnListfromwhitelist] pack:%s ack:%s", iter->packName, iter->actName);

        pPackList[i].scn_type     = SCN_PACK_HINT;
        pPackList[i].scn_state    = STATE_OFF;
        pPackList[i].pack_name[0] = '\0';
        pPackList[i].act_name[0]  = '\0';
        pPackList[i].fps[0]  = '\0';
        pPackList[i].window_mode[0]  = '\0';
        pPackList[i].sbe_featurename[0]  = '\0';
        pPackList[i].screen_off_action = MTKPOWER_SCREEN_OFF_WAIT_RESTORE;
        set_str_cpy(pPackList[i].pack_name, iter->packName, PACK_NAME_MAX);
        set_str_cpy(pPackList[i].act_name, iter->actName, CLASS_NAME_MAX);
        set_str_cpy(pPackList[i].sbe_featurename, iter->sbefeatureName, SBE_NAME_MAX);
        set_str_cpy(pPackList[i].fps, iter->fps, PACK_NAME_MAX);
        set_str_cpy(pPackList[i].window_mode, iter->window_mode, PACK_NAME_MAX);
        for(j = 0; iter != mXmlActList[i].end(); iter++, j++) {
            ALOGD("[updateScnListfromwhitelist] scn:%d pack:%s ack:%s fps:%s window:%s type:%s cmd:%s param:%d",
                i, iter->packName, iter->actName, iter->fps, iter->window_mode, iter->sbefeatureName, iter->cmd, iter->param1);
            cmdSetting(-1, iter->cmd, &pPackList[i], iter->param1, &rsc);
        }
    }
}

int updateCusScnTable(const char *path)
{
    XMLDocument docXml;
    //SPD: add xml decode by fan.feng1 20220922 start
    XMLError errXml;
    char *buf_decode = NULL;
    if(tran_powerhal_encode) {
        buf_decode = get_decode_buf(path);
        errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    } else {
        errXml = docXml.LoadFile(path);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    int scn = 0;

    ALOGD("[updateCusScnTable]");

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal CusScnTable config file '%s'. Error: %s",
            __FUNCTION__, path, XMLDocument::ErrorIDToName(errXml));
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    } else {
        ALOGI("%s: load powerhal CusScnTable config succeed!", __FUNCTION__);
    }

    ALOGD("[updateCusScnTable] errXml:%d" , errXml);

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO scenario info ", __FUNCTION__);
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    }

    XMLElement *elmtScenario = elmtRoot->FirstChildElement("scenario");

    while (elmtScenario != NULL) {
        const char* hintname = elmtScenario->Attribute("powerhint");
        if (hintname != nullptr) {
            scn = getHintId(hintname);
            if (scn == -1) {
                scn = atoi(hintname);
                ALOGI("[updateCusScnTable] hint %s not found in HintMappingTbl, hint_id: %d", hintname, scn);
                if (!scn || scn < CUS_HINT_BASE || scn > CUS_HINT_MAX) {
                    ALOGE("[updateCusScnTable] invalid hint: %s, hint_id: %d", hintname, scn);
                    elmtScenario = elmtScenario->NextSiblingElement();
                    continue;
                }
                PowerScnTbl_append(hintname, scn);
            }
        }
        if (scn != -1)
            perfxml_read_cmddata(elmtScenario, scn);
        else
            ALOGI("[updateCusScnTable] unknow scn name:%s" , elmtScenario->Attribute("powerhint"));
        elmtScenario = elmtScenario->NextSiblingElement();
    }
    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode && buf_decode) {
        free(buf_decode);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    return 0;
}

int loadConTable(const char *file_name)
{
     XMLDocument docXml;
    //SPD: add xml decode by fan.feng1 20220922 start
    XMLError errXml;
    char *buf_decode = NULL;
    if(tran_powerhal_encode) {
        buf_decode = get_decode_buf(file_name);
        errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    } else {
        errXml = docXml.LoadFile(file_name);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    const char* str = NULL;
    const char* id = NULL;
    int cmdid = 0;
    int idx = 0;

    ALOGD("[loadConTable]");

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal ConTable config file '%s'. Error: %s",
            __FUNCTION__, file_name, XMLDocument::ErrorIDToName(errXml));
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    } else {
        ALOGI("%s: load powerhal ConTable config succeed!", __FUNCTION__);
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO CMD info ", __FUNCTION__);
        //SPD: add xml decode by fan.feng1 20220922 start
        if(tran_powerhal_encode && buf_decode) {
            free(buf_decode);
        }
        //SPD: add xml decode by fan.feng1 20220922 end
        return 0;
    }

    XMLElement *elmtCMD = elmtRoot->FirstChildElement("CMD");

    while (elmtCMD) {
        str = elmtCMD->Attribute("name");
        id = elmtCMD->Attribute("id");
        if (id != NULL)
            cmdid = strtol(id, NULL, 16);
        tConTable[idx].cmdName = str;
        tConTable[idx].cmdID = cmdid;
        ALOGD("[loadConTable][%d] str:%s id:%s cmdid:%x", idx, str, id, cmdid);

        XMLElement *elmtEntry = elmtCMD->FirstChildElement("Entry");
        if (elmtEntry) {
            const char* path = elmtEntry->Attribute("path");
            ALOGD("[loadConTable][%d] path:%s ", idx, path);

            tConTable[idx].entry = path;

            if(path != NULL && access(path, W_OK) != -1)
                tConTable[idx].isValid = 0;
            else {
                tConTable[idx].isValid = -1;
                ALOGE("%s cannot access!!!!", tConTable[idx].cmdName.c_str());
                ALOGD("write of %s failed: %s\n", tConTable[idx].entry.c_str(), strerror(errno));
            }
        }

        XMLElement *elmtValid = elmtCMD->FirstChildElement("Valid");

        if (elmtValid != NULL && elmtValid->GetText() != nullptr) {
            tConTable[idx].ignore = atoi(elmtValid->GetText());
            if (tConTable[idx].ignore == 1) {
                tConTable[idx].isValid = 0;
                ALOGD("[loadConTable][%d] ignore:%d isValid:%d ",
                    idx, tConTable[idx].ignore, tConTable[idx].isValid);
            }
        } else {
            ALOGD("Valid is empty");
            tConTable[idx].ignore = 0;
        }

        XMLElement *elmtLegacyCmdID = elmtCMD->FirstChildElement("LegacyCmdID");

        if (elmtLegacyCmdID != NULL && elmtLegacyCmdID->GetText() != nullptr) {
            tConTable[idx].legacyCmdID = atoi(elmtLegacyCmdID->GetText());
            ALOGD("[loadConTable][%d] LegacyCmdID:%d ", idx ,tConTable[idx].legacyCmdID);
        } else {
            ALOGD("legacyCmdID value is empty");
            tConTable[idx].legacyCmdID = -1;
        }

        XMLElement *elmtCompare = elmtCMD->FirstChildElement("Compare");

        if (elmtCompare != NULL) {
            tConTable[idx].comp = elmtCompare->GetText();
            ALOGD("[loadConTable][%d] Compare:%s ", idx ,tConTable[idx].comp.c_str());
        } else {
            ALOGD("compare value is empty");
            tConTable[idx].comp.assign("");
        }

        XMLElement *elmtMaxVal = elmtCMD->FirstChildElement("MaxValue");

        if (elmtMaxVal != NULL && elmtMaxVal->GetText() != nullptr) {
            tConTable[idx].maxVal = atoi(elmtMaxVal->GetText());
            ALOGD("[loadConTable][%d] MaxValue:%d ", idx ,tConTable[idx].maxVal);
        } else {
            ALOGD("MaxValue value is empty");
            tConTable[idx].maxVal = 0;
        }

        XMLElement *elmtMinVal = elmtCMD->FirstChildElement("MinValue");

        if (elmtMinVal != NULL && elmtMinVal->GetText() != nullptr) {
            tConTable[idx].minVal = atoi(elmtMinVal->GetText());
            ALOGD("[loadConTable][%d] MinValue:%d ", idx, tConTable[idx].minVal);
        } else {
            ALOGD("MinValue is empty");
            tConTable[idx].minVal = 0;
        }

        XMLElement *elmtDefaultVal = elmtCMD->FirstChildElement("DefaultValue");

        if (elmtDefaultVal != NULL && elmtDefaultVal->GetText() != nullptr) {
            tConTable[idx].normalVal = atoi(elmtDefaultVal->GetText());
            ALOGD("[loadConTable][%d] DefaultValue:%d ", idx, tConTable[idx].normalVal);
        } else {
            ALOGD("DefaultValue is empty");
            tConTable[idx].normalVal = CFG_TBL_INVALID_VALUE;
        }

        XMLElement *elmtSportVal = elmtCMD->FirstChildElement("SportValue");

        if (elmtSportVal != NULL && elmtSportVal->GetText() != nullptr) {
            tConTable[idx].sportVal = atoi(elmtSportVal->GetText());
            ALOGD("[loadConTable][%d] sportVal:%d ", idx, tConTable[idx].sportVal);
        } else {
            ALOGD("SportVal is empty");
            tConTable[idx].sportVal = CFG_TBL_INVALID_VALUE;
        }

        XMLElement *elmtPrefix = elmtCMD->FirstChildElement("Prefix");

        if (elmtPrefix != NULL) {
            tConTable[idx].prefix = elmtPrefix->GetText();
            ALOGD("[loadConTable][%d] Prefix:%s ", idx, tConTable[idx].prefix.c_str());
        } else {
            ALOGD("prefix is empty");
            tConTable[idx].prefix.assign("");
        }

        if(tConTable[idx].prefix.length() != 0) {
            //ALOGD("[loadConTable] cmd:%s, path:%s, prefix 1:%s;", tConTable[idx].cmdName.c_str(),
            //    tConTable[idx].entry.c_str(), tConTable[idx].prefix.c_str());
            // Support one space. Use '^' to instead of ' ', i.e, "test^" => "test ".
            std::size_t found;
            do {
                found = tConTable[idx].prefix.find_first_of('^');
                if(found != std::string::npos)
                    tConTable[idx].prefix.replace(found, 1, " ");
            }
            while (found != std::string::npos);

            ALOGD("[loadConTable] cmd:%s, path:%s, prefix 2:%s;", tConTable[idx].cmdName.c_str(),
            tConTable[idx].entry.c_str(), tConTable[idx].prefix.c_str());
        }
        XMLElement *elmtInitWrite = elmtCMD->FirstChildElement("InitWrite");
        if (elmtInitWrite != NULL && elmtInitWrite->GetText() != nullptr) {
            tConTable[idx].init_set_default = atoi(elmtInitWrite->GetText());
            ALOGD("[loadConTable][%d] InitWrite:%d ", idx, tConTable[idx].init_set_default);
        } else {
            ALOGD("InitWrite is empty");
            tConTable[idx].init_set_default = 0;
        }
        if (tConTable[idx].normalVal != CFG_TBL_INVALID_VALUE) {
            tConTable[idx].defaultVal = tConTable[idx].normalVal;

            if(tConTable[idx].isValid == 0) {
                if(tConTable[idx].prefix.length() == 0 && tConTable[idx].normalVal > 0) {
                     if (tConTable[idx].entry.length() > 0) {
                        if (tConTable[idx].init_set_default == 1) {
                            set_value(tConTable[idx].entry.c_str(), tConTable[idx].normalVal);
                        }
                    }
                } else {
                    char inBuf[64];
                    snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), tConTable[idx].normalVal);
                    if (tConTable[idx].entry.length() > 0) {
                        if (tConTable[idx].init_set_default == 1) {
                            set_value(tConTable[idx].entry.c_str(), inBuf);
                        }
                    }
                }
            }
        }
        else {
            if(tConTable[idx].isValid == 0) {
                if (tConTable[idx].entry.length() > 0)
                    tConTable[idx].defaultVal = get_int_value(tConTable[idx].entry.c_str());
            }
        }

        ALOGD("[loadConTable] cmd:%s, path:%s, normal:%d, default:%d, init_set_default:%d", tConTable[idx].cmdName.c_str(),
            tConTable[idx].entry.c_str(), tConTable[idx].normalVal, tConTable[idx].defaultVal, tConTable[idx].init_set_default);

        // initial setting should be an invalid value
        tConTable[idx].gameVal = CFG_TBL_INVALID_VALUE;
        if(tConTable[idx].comp == LESS)
            tConTable[idx].resetVal = tConTable[idx].maxVal + 1;
        else
            tConTable[idx].resetVal = tConTable[idx].minVal - 1;

        tConTable[idx].curVal = tConTable[idx].resetVal;

        idx++;
        elmtCMD = elmtCMD->NextSiblingElement();
    }

    //SPD: add xml decode by fan.feng1 20220922 start
    if(tran_powerhal_encode && buf_decode) {
        free(buf_decode);
    }
    //SPD: add xml decode by fan.feng1 20220922 end
    return 1;
}

void perfservice_xmlparse_freeList()
{
    if(mXmlActList.size() > 0) {
        mXmlActList.clear();
        mXmlActList.shrink_to_fit();
    }
}

const char* perfservice_xmlparse_get_applist_file()
{
    const char * file_path;

    if (access(APP_LIST_XMLPATH, F_OK) != -1) {
        file_path = APP_LIST_XMLPATH;
    } else if (access(PACK_LIST_XMLPATH, F_OK) != -1) {
        file_path = PACK_LIST_XMLPATH;
    } else {
        file_path = NULL;
        ALOGE("can't find app list file\n");
    }

    return file_path;
}

const char* perfservice_xmlparse_get_data_applist_file()
{
    const char * file_path;

    if (access(APP_LIST_XMLPATH_2, F_OK) != -1) {
        file_path = APP_LIST_XMLPATH_2;
    } else if (access(PACK_LIST_XMLPATH_2, F_OK) != -1) {
        file_path = PACK_LIST_XMLPATH_2;
    } else {
        file_path = NULL;
        ALOGE("can't find app list file\n");
    }

    return file_path;
}

int perfservice_xmlparse_reload_whitelist()
{
    int PackNum = 0;

    const char * app_file_path;
    const char * data_app_file_path;

    if (check_data_config_validation() == 0)
        return -1;

    app_file_path = perfservice_xmlparse_get_applist_file();
    data_app_file_path = perfservice_xmlparse_get_data_applist_file();

    if (app_file_path == NULL)
        return -1;

    PackNum = update_activity_whitelist_table(app_file_path, XMLPARSE_ACTION_MERGE);
    ALOGI("[init] nPackNum:%d", PackNum);

    if (data_app_file_path != NULL && access(data_app_file_path, F_OK) != -1) {
        PackNum = update_activity_whitelist_table(data_app_file_path, XMLPARSE_ACTION_REPLACE);
        ALOGI("[init] reloaed nPackNum:%d", PackNum);
    } else {
        ALOGI("access of %s failed 1: %s\n", data_app_file_path, strerror(errno));
    }

    if(PackNum < 0) {
        ALOGE("invalid nPackNum");
        return 0;
    }

    return PackNum;

}

static int check_data_config_validation()
{
    struct stat stat_buf;
    time_t data_mtime = 0, system_mtime = 0;

    const char * app_file_path;
    const char * data_app_file_path;

    app_file_path = perfservice_xmlparse_get_applist_file();
    data_app_file_path = perfservice_xmlparse_get_data_applist_file();

    if (data_app_file_path != NULL && 0 == stat(data_app_file_path, &stat_buf)) {
        data_mtime = stat_buf.st_mtime;
    } else {
        ALOGI("check_data_config_validation: no data");
        return 0;
    }

    if (app_file_path != NULL && 0 == stat(app_file_path, &stat_buf)) {
        system_mtime = stat_buf.st_mtime;
    }

    if (data_mtime > system_mtime) {
        ALOGI("check_data_config_validation: true");
        return 1;
    }
    else {
        ALOGI("check_data_config_validation: false data_mtime:%ld system_mtime:%ld", data_mtime, system_mtime);
        return 0;
    }
}

int perfservice_xmlparse_init()
{
    int PackNum = 0;

    const char * app_file_path = NULL;
    const char * data_app_file_path = NULL;

    app_file_path = perfservice_xmlparse_get_applist_file();
    data_app_file_path = perfservice_xmlparse_get_data_applist_file();

    if (app_file_path == NULL)
        return 0;

    PackNum = update_activity_whitelist_table(app_file_path, XMLPARSE_ACTION_MERGE);

    ALOGI("[init] nPackNum:%d", PackNum);


    if (data_app_file_path != NULL && access(data_app_file_path, F_OK) != -1 && check_data_config_validation() == 1) {
        PackNum = update_activity_whitelist_table(data_app_file_path, XMLPARSE_ACTION_REPLACE);
        ALOGI("[init] reloaed nPackNum:%d", PackNum);
    } else {
        ALOGI("access of %s failed 1: %s\n", data_app_file_path, strerror(errno));
    }

    if(PackNum < 0) {
        ALOGE("invalid nPackNum");
        return 0;
    }

    return PackNum;
}


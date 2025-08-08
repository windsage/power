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
#include <unordered_map>

#include "common.h"
#include "tran_common.h"
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
#define DL_PWR_SIZE_LIMIT 1024*1024

#define APP_LIST_XMLPATH        "/vendor/etc/power_app_cfg.xml"
#define APP_LIST_XMLPATH_2      "/data/vendor/powerhal/power_app_cfg.xml"

#define PACK_LIST_XMLPATH       "/vendor/etc/power_whitelist_cfg.xml"
//#define PACK_LIST_XMLPATH_2     "/data/system/power_whitelist_cfg.xml"
#define PACK_LIST_XMLPATH_2     "/data/vendor/powerhal/power_whitelist_cfg.xml"
#define POWER_DL_XML_XML        "/data/vendor/powerhal/powerdlttable.xml"

#define DEBUG_SMART_LAUNCH             0

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
std::unordered_map<std::string, int> gSmartLaunchPackageInfo;
const string DL_KEY("ZWX");

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
        ALOGI("%s, %d, %s, %d, %d, %s, %d, %d %d", tConTable[idx].cmdName.c_str(),  tConTable[idx].cmdID, tConTable[idx].entry.c_str(), tConTable[idx].defaultVal,
                tConTable[idx].curVal, tConTable[idx].comp.c_str(), tConTable[idx].maxVal, tConTable[idx].minVal, tConTable[idx].isValid);
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

int updateCusScnTable(const char *path)
{
    //SPD: add by rui.zhou6 by encode at 20250519 start
    XMLDocument docXml;
    XMLError errXml;
    char *buf_decode = NULL;
    buf_decode = get_decode_buf(path);
    errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    //errXml = docXml.LoadFile(path);
    //SPD: add by rui.zhou6 by encode at 20250519 end
    int scn = 0;

    ALOGD("[updateCusScnTable]");

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal CusScnTable config file '%s'. Error: %s",
            __FUNCTION__, path, XMLDocument::ErrorIDToName(errXml));
        //SPD: add by rui.zhou6 by encode at 20250519 start
        if(buf_decode) {
            free(buf_decode);
        }
        //SPD: add by rui.zhou6 by encode at 20250519 end
        return 0;
    } else {
        ALOGI("%s: load powerhal CusScnTable config succeed!", __FUNCTION__);
    }

    ALOGD("[updateCusScnTable] errXml:%d" , errXml);

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO scenario info ", __FUNCTION__);
        //SPD: add by rui.zhou6 by encode at 20250519 start
        if(buf_decode) {
            free(buf_decode);
        }
        //SPD: add by rui.zhou6 by encode at 20250519 end
        return 0;
    }

    XMLElement *elmtScenario = elmtRoot->FirstChildElement("scenario");

    while (elmtScenario != NULL) {
        const char* hintname = elmtScenario->Attribute("powerhint");
        if (hintname != nullptr) {
            scn = getHintId(hintname);
            if (scn == -1) {
                scn = strtol(hintname, NULL, 10);
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
    //SPD: add by rui.zhou6 by encode at 20250519 start
    if(buf_decode) {
        free(buf_decode);
    }
    //SPD: add by rui.zhou6 by encode at 20250519 end
    return 0;
}

std::vector<char> xor_encrypt_decrypt(const std::vector<char>& data, const std::string& key) {
    std::vector<char> output(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        output[i] = data[i] ^ key[i % key.length()];
    }
    return output;
}

std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return std::vector<char>();
    }
    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return data;
}

void write_file(const std::string& filename, const std::vector<char>& data) {
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), data.size());
    file.close();
}

int parsePackInfo(const char *file_name)
{
    XMLDocument docXml;
    XMLError errXml;
    const char* str = NULL;
    const char* id = NULL;
    int cmdid = 0;
    int idx = 0;

    ALOGI("[parsePackInfo]");
    std::vector<char> encrypted_data = read_file(file_name);
    if (encrypted_data.empty()) {
        ALOGE("%s: load %s fail!", __FUNCTION__, file_name);
        return -1;
    }
    std::vector<char> decrypted_data = xor_encrypt_decrypt(encrypted_data, DL_KEY);
    std::string decrypted_xml_content(decrypted_data.begin(), decrypted_data.end());

    errXml = docXml.Parse(decrypted_xml_content.c_str());

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal DlTable config file '%s'. Error: %s",
            __FUNCTION__, file_name, XMLDocument::ErrorIDToName(errXml));
        return -1;
    } else {
        ALOGI("%s: load powerhal PowerDlTable config succeed!", __FUNCTION__);
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO PowerDlTable info ", __FUNCTION__);
        return -1;
    }

    XMLElement *package = elmtRoot->FirstChildElement("Package");

    while (package != NULL) {
        const char* packageName = package->Attribute("name");
        if (packageName) {
            XMLElement* power = package->FirstChildElement("Power");
            if (power) {
                int value;
                if (power->QueryIntAttribute("value", &value) == XML_SUCCESS) {
                    gSmartLaunchPackageInfo[packageName] = value;
                }
            }
        }

        package = package->NextSiblingElement();

    }
    ALOGI("dlt table package sucess ");
    return 0;
}

int creatPowerDlXml(const char *save_name) {
    struct stat buffer;
    const char* powerDl_xml_content =
    "<DYNAMIC>\n"
    "    <Package name=\"This File is auto generated, NOT modify\">\n"
    "        <Power value=\"15\">\n"
    "        </Power>\n"
    "    </Package>\n"
    "</DYNAMIC>\n";

    if (stat(save_name, &buffer) != 0) {

        tinyxml2::XMLDocument doc;
        doc.Parse(powerDl_xml_content);

        tinyxml2::XMLPrinter printer;
        doc.Print(&printer);
        std::string processed_xml_content = printer.CStr();
        std::vector<char> reencrypted_data = xor_encrypt_decrypt(std::vector<char>(processed_xml_content.begin(), processed_xml_content.end()), DL_KEY);

        write_file(save_name, reencrypted_data);

        chmod(save_name, 0660);
    }

    return 0;
}

int loadPowerDlTable(const char *file_name)
{

    if (access(file_name, F_OK) != -1) {
        parsePackInfo(file_name);
    } else {
        creatPowerDlXml(file_name);
    }
    return 0;
}

int insertNewElement2DlTable(const char *file_name, const char* packageName, const char* value)
{
    XMLDocument docXml;

    std::vector<char> encrypted_data = read_file(file_name);
    if (encrypted_data.empty()) {
        ALOGE("%s: load %s fail!", __FUNCTION__, file_name);
        return -1;
    }
    std::vector<char> decrypted_data = xor_encrypt_decrypt(encrypted_data, DL_KEY);
    std::string decrypted_xml_content(decrypted_data.begin(), decrypted_data.end());

    XMLError eResult = docXml.Parse(decrypted_xml_content.c_str());
    if (eResult != XML_SUCCESS) {
        ALOGE("Error loading XML file: %s !", file_name);
        return -1;
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot) {
        XMLElement* newPackage = docXml.NewElement("Package");
        newPackage->SetAttribute("name", packageName);

        XMLElement* newDynamic = docXml.NewElement("Power");
        newDynamic->SetAttribute("value", value);

        newPackage->InsertEndChild(newDynamic);
        elmtRoot->InsertEndChild(newPackage);
    } else {
        ALOGE("Error: Root element not found!: %s !", file_name);
        return -1;
    }

    tinyxml2::XMLPrinter printer;
    docXml.Print(&printer);
    std::string processed_xml_content = printer.CStr();
    if (processed_xml_content.size() >= DL_PWR_SIZE_LIMIT) {
        ALOGE("Error: PWR_DL exceeds the size limit of %d, but now size is !", DL_PWR_SIZE_LIMIT, processed_xml_content.size());
        return -1;
    }

    std::vector<char> reencrypted_data = xor_encrypt_decrypt(std::vector<char>(processed_xml_content.begin(), processed_xml_content.end()), DL_KEY);
    write_file(file_name, reencrypted_data);

#if DEBUG_SMART_LAUNCH
    eResult = docXml.SaveFile(POWER_DL_XML_XML);
    if (eResult != XML_SUCCESS) {
        ALOGE("Error saving XML file: %s !", POWER_DL_XML_XML);
        return -1;
    }
    ALOGI("insertNewElement2DlTable sucess: %s !", POWER_DL_XML_XML);
#endif

    return 0;
}

int deleteElement2DlTable(const char *file_name, const char* packageName)
{
    XMLDocument docXml;

    std::vector<char> encrypted_data = read_file(file_name);
    if (encrypted_data.empty()) {
        ALOGE("%s: load %s fail!", __FUNCTION__, file_name);
        return -1;
    }
    std::vector<char> decrypted_data = xor_encrypt_decrypt(encrypted_data, DL_KEY);
    std::string decrypted_xml_content(decrypted_data.begin(), decrypted_data.end());

    XMLError eResult = docXml.Parse(decrypted_xml_content.c_str());
    if (eResult != XML_SUCCESS) {
        ALOGE("deleteElement2DlTable Error loading XML file: %s !", file_name);
        return -1;
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO PowerDlTable info ", __FUNCTION__);
        return -1;
    }

    XMLElement* package = elmtRoot->FirstChildElement("Package");
    while (package) {
        const char* packname = package->Attribute("name");
        if (packname && strcmp(packname, packageName) == 0) {
                elmtRoot->DeleteChild(package);
                break;
        }
        package = package->NextSiblingElement();
    }

    tinyxml2::XMLPrinter printer;
    docXml.Print(&printer);
    std::string processed_xml_content = printer.CStr();
    std::vector<char> reencrypted_data = xor_encrypt_decrypt(std::vector<char>(processed_xml_content.begin(), processed_xml_content.end()), DL_KEY);

    write_file(file_name, reencrypted_data);

#if DEBUG_SMART_LAUNCH
    eResult = docXml.SaveFile(POWER_DL_XML_XML);
    if (eResult != XML_SUCCESS) {
        ALOGE("Error saving XML file: %s !", POWER_DL_XML_XML);
        return -1;
    }
    ALOGI("deleteElement2DlTable sucess: %s !", POWER_DL_XML_XML);
#endif

    return 0;
}

int modifyElement2DlTable(const char *file_name, const char* packageName, const char* new_val)
{
    XMLDocument docXml;

    std::vector<char> encrypted_data = read_file(file_name);
    if (encrypted_data.empty()) {
        ALOGE("%s: load %s fail!", __FUNCTION__, file_name);
        return -1;
    }
    std::vector<char> decrypted_data = xor_encrypt_decrypt(encrypted_data, DL_KEY);
    std::string decrypted_xml_content(decrypted_data.begin(), decrypted_data.end());

    XMLError eResult = docXml.Parse(decrypted_xml_content.c_str());
    if (eResult != XML_SUCCESS) {
        ALOGE("%s: Error loading XML file: %s !", __FUNCTION__, file_name);
        return -1;
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO PowerDlTable info ", __FUNCTION__);
        return -1;
    }

    XMLElement* package = elmtRoot->FirstChildElement("Package");
    while (package) {
        const char* packname = package->Attribute("name");
        if (packname && strcmp(packname, packageName) == 0) {
            XMLElement* newdynamic = package->FirstChildElement("Power");
            if (newdynamic) {
                newdynamic->SetAttribute("value", new_val);
                break;
            }
        }
        package = package->NextSiblingElement();
    }

    tinyxml2::XMLPrinter printer;
    docXml.Print(&printer);
    std::string processed_xml_content = printer.CStr();
    std::vector<char> reencrypted_data = xor_encrypt_decrypt(std::vector<char>(processed_xml_content.begin(), processed_xml_content.end()), DL_KEY);
    write_file(file_name, reencrypted_data);

#if DEBUG_SMART_LAUNCH
    eResult = docXml.SaveFile(POWER_DL_XML_XML);
    if (eResult != XML_SUCCESS) {
        ALOGE("Error saving XML file: %s !", POWER_DL_XML_XML);
        return -1;
    }
    ALOGI("modifyElement2DlTable sucess: %s !", POWER_DL_XML_XML);
#endif

    return 0;
}

int loadConTable(const char *file_name)
{
    //SPD: add by rui.zhou6 by encode at 20250519 start
    XMLDocument docXml;
    XMLError errXml;
    char *buf_decode = NULL;
    buf_decode = get_decode_buf(file_name);
    errXml = docXml.Parse(buf_decode, strlen(buf_decode));
    //errXml = docXml.LoadFile(file_name);
    //SPD: add by rui.zhou6 by encode at 20250519 end
    const char* str = NULL;
    const char* id = NULL;
    int cmdid = 0;
    int idx = 0;

    ALOGD("[loadConTable]");

    if (errXml != XML_SUCCESS) {
        ALOGE("%s: Unable to powerhal ConTable config file '%s'. Error: %s",
            __FUNCTION__, file_name, XMLDocument::ErrorIDToName(errXml));
        //SPD: add by rui.zhou6 by encode at 20250519 start
        if(buf_decode) {
            free(buf_decode);
        }
        //SPD: add by rui.zhou6 by encode at 20250519 end
        return 0;
    } else {
        ALOGI("%s: load powerhal ConTable config succeed!", __FUNCTION__);
    }

    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot == NULL) {
        ALOGE("%s: elmtRoot == NULL !!!! NO CMD info ", __FUNCTION__);
        //SPD: add by rui.zhou6 by encode at 20250519 start
        if(buf_decode) {
            free(buf_decode);
        }
        //SPD: add by rui.zhou6 by encode at 20250519 end
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
            tConTable[idx].ignore = strtol(elmtValid->GetText(), NULL, 10);
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
            tConTable[idx].legacyCmdID = strtol(elmtLegacyCmdID->GetText(), NULL, 10);
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
            tConTable[idx].maxVal = strtol(elmtMaxVal->GetText(), NULL, 10);
            ALOGD("[loadConTable][%d] MaxValue:%d ", idx ,tConTable[idx].maxVal);
        } else {
            ALOGD("MaxValue value is empty");
            tConTable[idx].maxVal = 0;
        }

        XMLElement *elmtMinVal = elmtCMD->FirstChildElement("MinValue");

        if (elmtMinVal != NULL && elmtMinVal->GetText() != nullptr) {
            tConTable[idx].minVal = strtol(elmtMinVal->GetText(), NULL, 10);
            ALOGD("[loadConTable][%d] MinValue:%d ", idx, tConTable[idx].minVal);
        } else {
            ALOGD("MinValue is empty");
            tConTable[idx].minVal = 0;
        }

        XMLElement *elmtDefaultVal = elmtCMD->FirstChildElement("DefaultValue");

        if (elmtDefaultVal != NULL && elmtDefaultVal->GetText() != nullptr) {
            tConTable[idx].defaultVal = strtol(elmtDefaultVal->GetText(), NULL, 10);
            ALOGD("[loadConTable][%d] DefaultValue:%d ", idx, tConTable[idx].defaultVal);
        } else {
            ALOGD("DefaultValue is empty");
            tConTable[idx].defaultVal = CFG_TBL_INVALID_VALUE;
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
            tConTable[idx].init_set_default = strtol(elmtInitWrite->GetText(), NULL, 10);
            ALOGD("[loadConTable][%d] InitWrite:%d ", idx, tConTable[idx].init_set_default);
        } else {
            ALOGD("InitWrite is empty");
            tConTable[idx].init_set_default = 0;
        }

        if (tConTable[idx].defaultVal != CFG_TBL_INVALID_VALUE) {
            if(tConTable[idx].isValid == 0) {
                if(tConTable[idx].prefix.length() == 0 && tConTable[idx].defaultVal > 0) {
                    if (tConTable[idx].entry.length() > 0) {
                        if (tConTable[idx].init_set_default == 1) {
                            set_value(tConTable[idx].entry.c_str(), tConTable[idx].defaultVal);
                        }
                    }
                } else {
                    char inBuf[64];
                    if(snprintf(inBuf, 64, "%s%d", tConTable[idx].prefix.c_str(), tConTable[idx].defaultVal) < 0) {
                        LOG_E("snprint error");
                    }
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

        tConTable[idx].priority = PRIORITY_LOWEST;

        ALOGD("[loadConTable] cmd:%s, path:%s, default:%d, init_set_default:%d, prirority: %d", tConTable[idx].cmdName.c_str(),
            tConTable[idx].entry.c_str(), tConTable[idx].defaultVal, tConTable[idx].init_set_default, tConTable[idx].priority);

        // initial setting should be an invalid value
        if(tConTable[idx].comp == LESS)
            tConTable[idx].resetVal = tConTable[idx].maxVal + 1;
        else
            tConTable[idx].resetVal = tConTable[idx].minVal - 1;

        tConTable[idx].curVal = tConTable[idx].resetVal;

        idx++;
        elmtCMD = elmtCMD->NextSiblingElement();
    }
    //SPD: add by rui.zhou6 by encode at 20250519 start
    if(buf_decode) {
        free(buf_decode);
    }
    //SPD: add by rui.zhou6 by encode at 20250519 end
    return 1;
}



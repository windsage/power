import os
import re
import xml.etree.ElementTree as ET
import argparse
import csv

csv_path = "../../power/include/command.csv"
config_path = os.path.abspath(r"..") + "/config"
rsc_list = []

def check_APPlist(root):
    error_found = 0
    package_list = []

    for p in root.findall('Package'):
        package_name = p.get('name')
        if (check_whitespace(package_name) == 0) or (check_same_item(package_name, package_list) == 0):
            print(p.attrib)
            error_found = 1

        package_list.append(package_name)

        for a in p.findall('Activity'):
            activity_name = a.get('name')
            if check_whitespace(activity_name) == 0:
                print(a.attrib)
                error_found = 1

            fps_count = 0
            window_count = 0
            fps_list = []
            fps_common_get = 0
            fps_common_other = 0
            for f in a.findall('FPS'):
                fps_name = f.get('value')
                if fps_common_other > 0 and fps_common_get == 0:
                    if fps_name == 'Common':
                        print("Error: package_name does not contains <FPS value=\"Common\"> first!", package_name)
                        error_found = 1

                if fps_name == 'Common':
                    fps_common_get = 1
                else:
                    fps_common_other = fps_common_other + 1

                fps_count = fps_count + 1
                if (check_whitespace(fps_name) == 0) or (check_same_item(fps_name, fps_list) == 0):
                    print(f.attrib)
                    error_found = 1

                fps_list.append(fps_name)

                for w in f.findall('WINDOW'):
                    window_name = w.get('mode')
                    if (window_name != "Common" and window_name != "Single" and window_name != "Multi"):
                        print("errors found in package ", package_name)
                        print("wrong window name: ", window_name)
                        print("WINDOW mode should be Common/Single/Multi ")
                        error_found = 1
                    window_count = window_count + 1

                    fps_common_data_count = 0
                    cmd_list = []
                    for data in w.findall('data'):
                        fps_common_data_count = fps_common_data_count + 1
                        cmd = data.get('cmd')
                        if (check_whitespace(cmd) == 0) or (check_same_item(cmd, cmd_list) == 0) or (check_rsc_cmd(cmd) == 0):
                            print("Errors found in package ", package_name)
                            print(data.attrib)
                            error_found = 1

                        cmd_list.append(cmd)

                        if check_parameter(data) == 0:
                            print(data.attrib)
                            error_found = 1

                        if check_cmd_old_format(cmd) == 0:
                            print("Errors found in package ", package_name)
                            print(data.attrib)
                            error_found = 1

            if fps_count == 0:
                print("Errors found in package:", package_name, activity_name)
                print("Error: missing FPS_value")
                error_found = 1
            if window_count == 0:
                print("Errors found in package:", package_name, activity_name)
                print("Error: missing WINDOW_mode")
                error_found = 1

    if error_found == 1:
        return 0
    else:
        return 1

def check_whitespace(string_name):
    res = " " in string_name
    if(res):
        print("Error: string name contains whitespace character!")
        return 0
    return 1

def check_cmd_old_format(string_name):
    res = "_30" in string_name or "_60" in string_name or "_90" in string_name or "_120" in string_name or "_144" in string_name
    if(res):
        print("Error: string name contains _30/_60/_90/_120/_144 character!")
        return 0
    return 1

def is_same_pattern(pattern, text):
    regex_pattern = pattern.replace('*', '.*') + '$'
    regex = re.compile(regex_pattern)
    return bool(regex.match(text))

def check_same_item(item, item_list):
    if len(item_list) > 0:
        for i in range(len(item_list)):
            if is_same_pattern(item_list[i], item) is True or is_same_pattern(item, item_list[i]) is True:
                print("Error: same Package/Activity/FPS/cmd name!", item_list[i], item)
                return 0
    return 1

def load_csv_file():
    with open(csv_path) as f:
        reader = csv.reader(f)
        for row in reader:
            if row[0] != "COMMAND":
                #print(row[0])
                rsc_list.append(row[0])
    #print(len(rsc_list))

def check_parameter(data):
    if data.keys()[1] != "param1":
        print("Error: parameter's attribute should be named 'param1' ")
        return 0
    if data.get("param1") == "":
        print("Error: missing param1 value!")
        return 0
    return 1

def check_rsc_cmd(cmd):
    for i in range(len(rsc_list)):
        if cmd == rsc_list[i]:
            return 1
    print("Error: undefined resource cmd name! ")
    return 0

def parse_each_device():
    error_found = 0
    device_list = []
    file_list = os.listdir(config_path)

    for f in file_list:
        if "mt" in f:
            device_list.append(f)

    print(" ")

    for i in range(len(device_list)):
        xml_path = "../config/" + device_list[i] + "/app_list/power_app_cfg.xml"
        if os.path.isfile(xml_path) == False:
            continue
        #print(xml_path)
        r = ET.parse(xml_path).getroot()

        if check_APPlist(r) == 0:
            print(" ")
            print("Errors found in file:", xml_path)
            print("===================================================================")
            print(" ")
            error_found = 1

    if error_found == 1:
        return 0
    else:
        return 1

if __name__ == "__main__":
    load_csv_file()
    if parse_each_device() == 1:
        print("No error found.")
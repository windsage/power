LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ENCRYPT=$(MTK_PATH_SOURCE)/hardware/power_applist/tools/powerhal_encrypt.py
OUTFILE=$(LOCAL_PATH)/power_app.db
INFILE=$(LOCAL_PATH)/power_app_cfg.xml
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))
# Module name should match library/file name to be installed.
LOCAL_MODULE := power_app_cfg.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := power_app.db

# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)
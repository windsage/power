LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ENCRYPT=$(MTK_PATH_SOURCE)/hardware/power/tool/powerhal_encrypt.py
OUTFILE=$(LOCAL_PATH)/gbe.db
INFILE=$(LOCAL_PATH)/gbe.cfg
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))

# Module name should match library/file name to be installed.
LOCAL_MODULE := gbe.cfg
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := gbe.db
# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)

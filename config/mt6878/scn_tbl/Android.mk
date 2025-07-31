LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TRAN_POWERHAL_ENCODE_SUPPORT)),yes)
ENCRYPT=$(MTK_PATH_SOURCE)/hardware/power/tool/powerhal_encrypt.py
OUTFILE=$(LOCAL_PATH)/powerscntbl.db
INFILE=$(LOCAL_PATH)/powerscntbl.xml
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))
endif
# Module name should match library/file name to be installed.
LOCAL_MODULE := powerscntbl.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
ifeq ($(strip $(TRAN_POWERHAL_ENCODE_SUPPORT)),yes)
LOCAL_SRC_FILES := powerscntbl.db
else
LOCAL_SRC_FILES := $(LOCAL_MODULE)
endif
# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)

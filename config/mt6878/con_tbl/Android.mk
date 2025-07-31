LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TRAN_POWERHAL_ENCODE_SUPPORT)),yes)
ENCRYPT=$(MTK_PATH_SOURCE)/hardware/power/tool/powerhal_encrypt.py
OUTFILE=$(LOCAL_PATH)/powercontable.db
INFILE=$(LOCAL_PATH)/powercontable.xml
ifeq ($(findstring core_ctl, $(TARGET_PRODUCT)), core_ctl)
ifneq ($(wildcard $(LOCAL_PATH)/powercontable_core_ctl.xml),)
INFILE=$(LOCAL_PATH)/powercontable_core_ctl.xml
endif
endif
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))
endif
# Module name should match library/file name to be installed.
LOCAL_MODULE := powercontable.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional

ifeq ($(findstring core_ctl, $(TARGET_PRODUCT)), core_ctl)
ifneq ($(wildcard $(LOCAL_PATH)/powercontable_core_ctl.xml),)
ifeq ($(strip $(TRAN_POWERHAL_ENCODE_SUPPORT)),yes)
LOCAL_SRC_FILES := powercontable.db
else
LOCAL_SRC_FILES := powercontable_core_ctl.xml
endif
endif
else ifeq ($(strip $(TRAN_POWERHAL_ENCODE_SUPPORT)),yes)
LOCAL_SRC_FILES := powercontable.db
else
LOCAL_SRC_FILES := $(LOCAL_MODULE)
endif
# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)

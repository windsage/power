LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Module name should match library/file name to be installed.
LOCAL_MODULE := powercontable.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)

ifeq ($(findstring core_ctl, $(TARGET_PRODUCT)), core_ctl)
ifneq ($(wildcard $(LOCAL_PATH)/powercontable_core_ctl.xml),)
LOCAL_SRC_FILES := powercontable_core_ctl.xml
endif
endif

# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)

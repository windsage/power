
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := power_timer.cpp \
        mi_util.cpp \
        ports.cpp \
        ptimer.cpp \
         
LOCAL_SHARED_LIBRARIES := liblog \
        libdl \
        libutils \
        libcutils \

#LOCAL_HEADER_LIBRARIES += eara_internal_headers \
#        eara_public_headers

LOCAL_MODULE := libpower_timer
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTK_SHARED_LIBRARY)

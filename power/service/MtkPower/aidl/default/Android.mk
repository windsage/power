
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libpowerhal_headers \
        libpower_service_headers \
        libpowerhal_lib_headers

LOCAL_SRC_FILES := MtkPowerService.cpp \
        ../../../util/libpowerhal_wrap.cpp \

LOCAL_SHARED_LIBRARIES := liblog \
        libhardware \
        libhidlbase \
        libpowerhal \
        libutils \
        libcutils \
        libbinder_ndk \
        vendor.mediatek.hardware.mtkpower-V3-ndk \

LOCAL_MODULE := vendor.mediatek.hardware.mtkpower-aidl-impl
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER := mtk
include $(MTK_SHARED_LIBRARY)
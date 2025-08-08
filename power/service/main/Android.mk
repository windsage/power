
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libpower_service_headers \
    libpowerhal_headers \
    libpower_config_headers \
    power_timer_headers \

LOCAL_SRC_FILES := service.cpp \
         ../util/libpowerhal_wrap.cpp \
         ../util/libpowercloud.cpp \

LOCAL_SHARED_LIBRARIES := liblog \
        libdl \
        libutils \
        libcutils \
        libhardware \
        libhidlbase \
        libhardware_legacy \
        vendor.mediatek.hardware.mtkpower-V3-ndk \
        vendor.mediatek.hardware.mtkpower-aidl-impl \
        android.hardware.power-service-mediatek \
        libbase \
        libbinder_ndk \
        libtranlog \

LOCAL_SHARED_LIBRARIES += \
        android.hardware.power-V6-ndk

LOCAL_SHARED_LIBRARIES += \
        libpower_timer \

LOCAL_MODULE := vendor.mediatek.hardware.mtkpower-service.mediatek
LOCAL_INIT_RC := vendor.mediatek.hardware.mtkpower@1.0-service.rc \
                 vendor.mediatek.hardware.mtkpower@1.0-init.rc
LOCAL_VINTF_FRAGMENTS := power-mediatek.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER := mtk
include $(MTK_EXECUTABLE)


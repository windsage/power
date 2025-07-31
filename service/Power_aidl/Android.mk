
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libpowerhal_headers \
        libpower_config_headers \
        libpower_service_headers \


LOCAL_SRC_FILES := Power.cpp \
        PowerHintSession.cpp \
        PowerHintSessionManager.cpp \
        include/sysctl.cpp \
        ../util/libpowerhal_wrap.cpp \


LOCAL_SHARED_LIBRARIES := liblog \
        libbase \
        libbinder_ndk \

ifeq ($(PLATFORM_VERSION),12)
LOCAL_SHARED_LIBRARIES += \
        android.hardware.power-V4-ndk_platform
else
LOCAL_SHARED_LIBRARIES += \
        android.hardware.power-V4-ndk
endif

LOCAL_SHARED_LIBRARIES += \
        libutils \
        libcutils \
        libdrm \
        vendor.mediatek.hardware.mtkpower@1.0 \
        vendor.mediatek.hardware.mtkpower@1.1 \
        vendor.mediatek.hardware.mtkpower@1.2 \
        vendor.mediatek.hardware.mtkpower-V1-ndk \


LOCAL_MODULE := android.hardware.power-service-mediatek
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER := mtk
include $(MTK_SHARED_LIBRARY)


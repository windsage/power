
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := perfAPPListScenario.cpp \
                   common.cpp \
                   base64.cpp \

LOCAL_SHARED_LIBRARIES := libc libcutils libdl libui libutils liblog libexpat libtinyxml2\
    libhidlbase \
    libhardware \
    libpowerhal \
    vendor.mediatek.hardware.mtkpower@1.2 \

LOCAL_HEADER_LIBRARIES += libpowerhal_tinyxml2_headers libpowerhal_headers \

ifeq ($(HAVE_AEE_FEATURE),yes)
    LOCAL_SHARED_LIBRARIES += libaedv
    LOCAL_CFLAGS += -DHAVE_AEE_FEATURE
    LOCAL_HEADER_LIBRARIES += libaed_headers
endif

LOCAL_MODULE := lib_power_applist
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTK_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)

# workaround for tinyxml2
include $(CLEAR_VARS)
LOCAL_MODULE:= libpowerhal_tinyxml2_headers
LOCAL_EXPORT_C_INCLUDE_DIRS:= $(MTK_PATH_SOURCE)/external/tinyxml2 \
include $(BUILD_HEADER_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := perfservice.cpp \
    common.cpp \
    perfservice_xmlparse.cpp \
    perfservice_scn.cpp \
    powerhal_api.cpp \
    handler/power_msg_handler.cpp \
    handler/power_boot_handler.cpp \
    handler/power_touch_handler.cpp \
    handler/power_error_handler.cpp \
    ../util/mi_util.cpp \
    ../util/ptimer.cpp \
    ../util/ports.cpp \
    ../util/power_ipc.cpp \
    ../util/powerd.cpp \
    ../util/powerd_core.cpp \
    ../util/powerd_cmd.cpp \
    ../util/powerc.cpp \
    rsc_util/utility_thermal.cpp \
    rsc_util/utility_consys.cpp \
    rsc_util/utility_fps.cpp \
    rsc_util/utility_netd.cpp \
    rsc_util/utility_ux.c \
    rsc_util/utility_ril.c \
    rsc_util/utility_io.cpp \
    rsc_util/utility_touch.cpp \
    rsc_util/utility_sys.cpp \
    rsc_util/utility_consys_bt.cpp \
    rsc_util/utility_gpu.cpp \
    rsc_util/utility_dram.cpp \
    rsc_util/utility_video.cpp \
    rsc_util/utility_mem.cpp \
    rsc_util/utility_cpuset.cpp \
    rsc_util/utility_sbe_handle.cpp \
    rsc_util/utility_sf.cpp \
    rsc_util/utility_sched.cpp \
    rsc_util/utility_peak_power.cpp

LOCAL_SHARED_LIBRARIES := libc libcutils libdl libui libutils liblog libexpat libtinyxml2\
    libhidlbase \
    libhardware \
    vendor.mediatek.hardware.netdagent@1.0 \
    vendor.mediatek.hardware.netdagent-V1-ndk \
    vendor.mediatek.hardware.bluetooth.audio@2.1 \
    vendor.mediatek.hardware.bluetooth.audio@2.2 \
    vendor.mediatek.hardware.mtkpower@1.0 \
    vendor.mediatek.hardware.mtkpower@1.1 \
    vendor.mediatek.hardware.mtkpower@1.2 \
    libpower_timer \
    libged \
    libfmq \
    libbase \
    libbinder_ndk \
    libbinder \
    vendor.mediatek.framework.mtksf_ext-V2-ndk \
    android.hardware.bluetooth.audio-V4-ndk

LOCAL_HEADER_LIBRARIES += libpowerhal_lib_headers \
    libpowerhal_headers \
    libpower_service_headers \
    libpowerhal_tinyxml2_headers \
    power_timer_headers \
    libhardware_headers

ifeq ($(HAVE_AEE_FEATURE),yes)
    LOCAL_SHARED_LIBRARIES += libaedv
    LOCAL_CFLAGS += -DHAVE_AEE_FEATURE
    LOCAL_HEADER_LIBRARIES += libaed_headers
endif

LOCAL_MODULE := libpowerhal
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
include $(MTK_SHARED_LIBRARY)


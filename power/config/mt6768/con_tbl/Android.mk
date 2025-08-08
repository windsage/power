LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ENCRYPT=$(MTK_PATH_SOURCE)/hardware/power/tool/powerhal_encrypt.py
OUTFILE=$(LOCAL_PATH)/powercontable.db
INFILE=$(LOCAL_PATH)/powercontable.xml
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))

# Module name should match library/file name to be installed.
LOCAL_MODULE := powercontable.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional
ifeq ($(LINUX_KERNEL_VERSION), kernel-6.1)
LOCAL_SRC_FILES := powercontable_k610.xml
else
LOCAL_SRC_FILES := powercontable.db
endif
# set class according to lib/file attribute
LOCAL_MODULE_CLASS := ETC
include $(BUILD_PREBUILT)

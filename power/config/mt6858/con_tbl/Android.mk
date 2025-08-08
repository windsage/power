# 定义当前模块的路径
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# 定义加密脚本路径和输入/输出文件
ENCRYPT := $(MTK_PATH_SOURCE)/hardware/power/tool/powerhal_encrypt.py
OUTFILE := $(LOCAL_PATH)/powercontable.db
INFILE := $(LOCAL_PATH)/powercontable.xml

# 定义 core_ctl 文件的输入和输出路径
CORE_CTL_INFILE := $(LOCAL_PATH)/powercontable_core_ctl.xml
CORE_CTL_OUTFILE := $(LOCAL_PATH)/powercontable_core_ctl.db

# 调用加密脚本，将 powercontable.xml 加密为 powercontable.db
$(shell python3 $(ENCRYPT) -e $(INFILE) $(OUTFILE))

# 如果 powercontable_core_ctl.xml 文件存在，则对其进行加密
ifneq ($(wildcard $(CORE_CTL_INFILE)),)
    $(shell python3 $(ENCRYPT) -e $(CORE_CTL_INFILE) $(CORE_CTL_OUTFILE))
endif

# 定义模块名称和属性
LOCAL_MODULE := powercontable.xml
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_MODULE_TAGS := optional

# 检查是否启用了 core_ctl 特性
ifeq ($(findstring core_ctl, $(TARGET_PRODUCT)), core_ctl)
# 如果 core_ctl 特性启用，且存在加密后的 core_ctl 数据库文件，则使用该文件
ifneq ($(wildcard $(CORE_CTL_OUTFILE)),)
LOCAL_SRC_FILES := powercontable_core_ctl.db
else
# 否则使用加密后的默认数据库文件
LOCAL_SRC_FILES := powercontable.db
endif
else
# 如果 core_ctl 特性未启用，默认使用加密后的数据库文件
LOCAL_SRC_FILES := powercontable.db
endif

# 设置模块类别为 ETC
LOCAL_MODULE_CLASS := ETC
# 包含预构建模块的构建规则
include $(BUILD_PREBUILT)

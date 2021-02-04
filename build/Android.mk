LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DOS_LINUX -DOS_ANDROID  -pipe -fPIC -g -Wall -O0 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
LOCAL_LDLIBS += -lm -pthread -ldl

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../as_common/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../mov/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../rtmp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../rtsp

LOCAL_SRC_FILES := $(wildcard ../*.c)
LOCAL_SRC_FILES += $(wildcard ../*.cpp)
LOCAL_SRC_FILES += $(wildcard ../as_common/src/*.c)
LOCAL_SRC_FILES += $(wildcard ../as_common/src/*.cpp)
LOCAL_SRC_FILES += $(wildcard ../mov/source/*.c)
LOCAL_SRC_FILES += $(wildcard ../mov/source/*.cpp)
LOCAL_SRC_FILES += $(wildcard ../rtmp/*.c)
LOCAL_SRC_FILES += $(wildcard ../rtmp/*.cpp)
LOCAL_SRC_FILES += $(wildcard ../rtsp/*.c)
LOCAL_SRC_FILES += $(wildcard ../rtsp/*.cpp)

LOCAL_MODULE := MediaService
include $(BUILD_SHARED_LIBRARY)

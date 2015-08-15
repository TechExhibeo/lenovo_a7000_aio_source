# Copyright Statement:
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    rpserver.cpp

LOCAL_C_INCLUDES := \
    external/openssl/include \
    frameworks/base/include/androidfw

LOCAL_SHARED_LIBRARIES := \
    libpower \
    libcutils \
    libz \
    libcrypto \
    libandroidfw


PLATFORM_CERTIFICATE := $(dir $(DEFAULT_SYSTEM_DEV_CERTIFICATE))platform.x509.pem
$(warning "PLATFORM_CERTIFICATE=$(PLATFORM_CERTIFICATE)")
PLATFORM_CERTIFICATE_SERIAL := $(shell openssl x509 -in $(PLATFORM_CERTIFICATE) -noout -serial | cut -d = -f 2)
$(warning "PLATFORM_CERTIFICATE_SERIAL=$(PLATFORM_CERTIFICATE_SERIAL)")
ifneq (,$(strip $(PLATFORM_CERTIFICATE_SERIAL)))
LOCAL_CFLAGS += -DPLATFORM_CERTIFICATE_SERIAL=\"$(PLATFORM_CERTIFICATE_SERIAL)\"
endif

LOCAL_MODULE:= rpserver
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)

include $(BUILD_EXECUTABLE)

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := rpres
LOCAL_SRC_FILES :=  rpres
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
include $(BUILD_PREBUILT)
###############################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    rpclient.cpp

LOCAL_C_INCLUDES := \
    external/skia/include/core \
    external/skia/include/images \
    external/openssl/include

LOCAL_SHARED_LIBRARIES := \
    libgui \
    libutils \
    libskia \
    libcutils \
    libcrypto

LOCAL_MODULE:= rpclient
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)

include $(BUILD_EXECUTABLE)

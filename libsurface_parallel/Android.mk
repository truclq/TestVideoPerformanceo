LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
MYDROID_DIR=/home/ErickLe/Desktop/Working_ex/rcar_proj/kk_v140/20_work/android/mydroid-4.4.2_r2
LIBRARY_DIR=$(MYDROID_DIR)/out/target/product/koelsch/system/lib

LOCAL_SRC_FILES:= \
	display_omx.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
    libui \
    libgui
	#libomxr_core

LOCAL_MODULE:= libomx_surface_parallel
LOCAL_MODULE_TAGS := tests

LOCAL_CFLAGS = \
	-I$(MYDROID_DIR)/system/core/include              	\
	-I$(MYDROID_DIR)/hardware/libhardware/include     	\
	-I$(MYDROID_DIR)/bionic/libc/include     			\
    -I$(MYDROID_DIR)/bionic/libstdc++/include     		\
	-I$(MYDROID_DIR)/bionic/libc/arch-arm/include     	\
    -I$(MYDROID_DIR)/frameworks/native/include 	        \
    -I$(MYDROID_DIR)/frameworks/native/opengl/include 	\
    -I$(MYDROID_DIR)/bionic/libc/kernel/common \
    -I$(MYDROID_DIR)/bionic/libc/kernel/arch-arm
    
LOCAL_LDFLAGS :=-L$(LIBRARY_DIR)

include $(BUILD_SHARED_LIBRARY)

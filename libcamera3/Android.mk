# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_HAVE_IMX_CAMERA),true)

include $(CLEAR_VARS)

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_VENDOR_MODULE := true
LOCAL_C_INCLUDES += \
    $(IMX_LIB_PATH)/imx-lib/pxp \
    system/core/include \
    system/media/camera/include \
    external/jpeg \
    $(FSL_PROPRIETARY_PATH)/fsl-proprietary/include \
    device/fsl/common/kernel-headers \
    $(IMX_PATH)/imx/include \
    external/fsl_vpu_omx/OpenMAXIL/src/component/vpu_wrapper \
    $(FSL_IMX_OMX_PATH)/fsl_imx_omx/OpenMAXIL/src/component/vpu_wrapper \
    $(IMX_PATH)/imx/display/gralloc_v2 \
    system/core/libion/include

LOCAL_SRC_FILES := \
    CameraHAL.cpp \
    Camera.cpp \
    Metadata.cpp \
    Stream.cpp \
    VendorTags.cpp \
    CameraUtils.cpp \
    MessageQueue.cpp \
    VideoStream.cpp \
    JpegBuilder.cpp \
    Ov5640Csi.cpp \
    Ov5640Csi8MQ.cpp \
    Ov5640Mipi.cpp \
    Ov5642Csi.cpp \
    Max9286Mipi.cpp \
    YuvToJpegEncoder.cpp \
    NV12_resize.c \
    USPStream.cpp \
    DMAStream.cpp \
    UvcDevice.cpp \
    Uvc7ulpDevice.cpp \
    TVINDevice.cpp \
    TVIN8DvDevice.cpp \
    VADCTVINDevice.cpp \
    MMAPStream.cpp \
    TinyExif.cpp

ifeq ($(BOARD_HAVE_VPU),true)
    LOCAL_SRC_FILES += \
    UvcMJPGDevice.cpp \
    MJPGStream.cpp
endif

LOCAL_SHARED_LIBRARIES := \
    libcamera_metadata \
    libcutils \
    liblog \
    libsync \
    libutils \
    libc \
    libjpeg \
    libion \
    libbinder \
    libcamera_client \
    libhardware_legacy

ifneq ($(TARGET_FSL_IMX_2D),)
    LOCAL_SHARED_LIBRARIES += \
	     libg2d
    LOCAL_CFLAGS += -DTARGET_FSL_IMX_2D
endif

ifeq ($(BOARD_HAVE_VPU),true)
    LOCAL_SHARED_LIBRARIES += \
            lib_vpu_wrapper
    LOCAL_CFLAGS += -DBOARD_HAVE_VPU
endif

ifeq ($(HAVE_FSL_IMX_PXP),true)
    LOCAL_SHARED_LIBRARIES += \
            libpxp
            LOCAL_CFLAGS += -DHAVE_FSL_IMX_PXP
endif

ifeq ($(HAVE_FSL_IMX_IPU),true)
    LOCAL_CFLAGS += -DHAVE_FSL_IMX_IPU
endif

ifeq ($(PRODUCT_MODEL), SABREAUTO-MX6SX)
    LOCAL_CPPFLAGS += -DVADC_TVIN
endif

ifeq ($(BOARD_SOC_TYPE), IMX7ULP)
    LOCAL_CPPFLAGS += -DIMX7ULP_UVC
endif

LOCAL_CFLAGS += -Wall -Wextra -fvisibility=hidden

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif

# Copyright (C) 2009 The Android Open Source Project
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
#

# the purpose of this sample is to demonstrate how one can
# generate two distinct shared libraries and have them both
# uploaded in
#
LOCAL_PATH:= $(call my-dir)
#TARGET_PLATFORM := 'android-8'

#====================Call Engine====================
include $(CLEAR_VARS)
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -L$(LOCAL_PATH)/thirdparty/libitri-sip -llog
LOCAL_MODULE     := call_engine
LOCAL_SRC_FILES  := call_engine_factory.cpp  \
                    CallEngineJni.cpp \
                    call/mdp.cpp \
                    sip_call/itri_sip/itri_call.cpp \
                    sip_call/itri_sip/itri_callmanager.cpp \
                    sip_call/itri_sip/itri_utility.cpp \
                    sip_call/itri_sip/stream_manager.cpp \
                    sip_call/itri_sip/itri_callback.cpp \
                    sip_call/itri_sip/itri_call_engine.cpp \
                    sip_call/itri_sip/srtp.cpp \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/call/ \
                    $(LOCAL_PATH)/sip_call/ \
                    $(LOCAL_PATH)/../icl-sip/ \
                    $(LOCAL_PATH)/../logger/ \

LOCAL_SHARED_LIBRARIES := libcm liblow libsipccl libsdpccl libadt libsipTx libDlgLayer libcallcontrol libSessAPI libMsgSummary libSessAPI libssl libcrypto
LOCAL_CFLAGS           := -D_ANDROID_APP -DICL_TLS
LOCAL_STATIC_LIBRARIES := liblogger
LOCAL_LDLIBS           := -L$(SYSROOT)/usr/lib -lm -lz
include $(BUILD_SHARED_LIBRARY)


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
TARGET_PLATFORM := 'android-8'
#====================Call Engine====================
include $(CLEAR_VARS)
SOFIA_SIP := thirdparty/libsofia-sip
SOFIA_SIP_UA := $(LOCAL_PATH)/$(SOFIA_SIP)/sofia-sip-1.12.11/libsofia-sip-ua

SOFIA_SIP_INCLUDES := $(LOCAL_PATH)/thirdparty/libsofia-sip/sofia-sip-1.12.11 \
                    $(SOFIA_SIP_UA)/bnf \
                    $(SOFIA_SIP_UA)/features \
                    $(SOFIA_SIP_UA)/http \
                    $(SOFIA_SIP_UA)/ipt \
                    $(SOFIA_SIP_UA)/iptsec \
                    $(SOFIA_SIP_UA)/msg \
                    $(SOFIA_SIP_UA)/nea \
                    $(SOFIA_SIP_UA)/nta \
                    $(SOFIA_SIP_UA)/nth \
                    $(SOFIA_SIP_UA)/nua \
                    $(SOFIA_SIP_UA)/sdp \
                    $(SOFIA_SIP_UA)/sip \
                    $(SOFIA_SIP_UA)/soa \
                    $(SOFIA_SIP_UA)/sresolv \
                    $(SOFIA_SIP_UA)/stun \
                    $(SOFIA_SIP_UA)/su \
                    $(SOFIA_SIP_UA)/tport \
                    $(SOFIA_SIP_UA)/url

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_MODULE    := call_engine

LOCAL_SRC_FILES := call/mdp.cpp \
                   sip_call/sip_call.cpp \
                   sip_call/sip_call_engine.cpp \
                   call_engine_factory.cpp  \
                   CallEngineJni.cpp 


LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)

LOCAL_C_INCLUDES += $(LOCAL_PATH) \
                    $(LOCAL_PATH)/call \
                    $(LOCAL_PATH)/sip_call 

LOCAL_STATIC_LIBRARIES := sofia-sip

#LOCAL_CFLAGS += $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_LDLIBS += -lm -lz #-lsofia-sip -L$(LOCAL_PATH)/$(SOFIA_SIP)
include $(BUILD_SHARED_LIBRARY)

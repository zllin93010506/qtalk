# LOCAL_PATH is one of libavutil, libavcodec, libavformat, or libswscale

#include $(LOCAL_PATH)/../config-$(TARGET_ARCH).mak
include $(LOCAL_PATH)/../config.mak

OBJS :=
OBJS-yes :=
MMX-OBJS-yes :=
include $(LOCAL_PATH)/Makefile

# collect objects
OBJS-$(HAVE_MMX) += $(MMX-OBJS-yes)
OBJS += $(OBJS-yes)

FFNAME := lib$(NAME)
FFLIBS := $(foreach,NAME,$(FFLIBS),lib$(NAME))
FFCFLAGS  = -DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign
FFCFLAGS += -DTARGET_CONFIG=\"config-$(TARGET_ARCH).h\"

ALL_S_FILES := $(wildcard $(LOCAL_PATH)/$(TARGET_ARCH)/*.S)
# find all *.S under $(LOCAL_PATH)/$(TARGET_ARCH)/
ALL_S_FILES := $(addprefix $(TARGET_ARCH)/, $(notdir $(ALL_S_FILES)))
#eliminate directory path and add prefix $(TARGET_ARCH)

ifneq ($(ALL_S_FILES),)
#if ALL_S_FILES is not empty
ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
#change format : all .S as .o
C_OBJS := $(filter-out $(ALL_S_OBJS),$(OBJS))
#filter out *.o in $OBJS
S_OBJS := $(filter $(ALL_S_OBJS),$(OBJS))
#filter  *.o in $OBJS
else
#if ALL_S_FILES is empty
C_OBJS := $(OBJS)
#assign C_OBJS = OBJS
S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
#replace all .o file in C_OBJS as .c
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))
#replace all .o file in S_OBJS as .S

### Lawrence modified

#NOT_CFILE := $(filter $(TARGET_ARCH)/qri_ppo_qta15_ffmpeg_VideoProcess.c,$(C_OBJS))
QRI_SRC := qri_ppo_qta15_ffmpeg_VideoProcess.c
QRI_OBJ := qri_ppo_qta15_ffmpeg_VideoProcess.o
C_FILES_2 := $(notdir $(C_FILES))
NOT_CFILE := $(filter $(QRI_SRC) ,$(C_FILES_2))
NOT_CFILE2 := $(addprefix $(TARGET_ARCH)/, $(NOT_CFILE))


C_FILES := $(filter-out qri_ppo_qta15_ffmpeg_VideoProcess.c,$(C_FILES))
C_OBJS := $(filter-out $(TARGET_ARCH)/qri_ppo_qta15_ffmpeg_VideoProcess.o,$(C_OBJS))
S_FILES := $(filter-out $(TARGET_ARCH)/qri_ppo_qta15_ffmpeg_VideoProcess.,$(S_FILES))

###Lawrence Modified end 

FFFILES := $(sort $(S_FILES)) $(sort $(C_FILES))

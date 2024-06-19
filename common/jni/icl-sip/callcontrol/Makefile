###############################################################################
#
# Makefile for common module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = callctrl_msg.o \
	callctrl_sipmsg.o \
	callctrl_Cont.o \
	callctrl_user.o \
	callctrl_call.o

TARGET = callctrl
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX -DICL_NO_MATCH_NOTIFY

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk

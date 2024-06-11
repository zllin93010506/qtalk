###############################################################################
#
# Makefile for common module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = dlg_msg.o \
	dlg_sipmsg.o \
	dlg_util.o

TARGET = DlgLayer
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX -DICL_NO_MATCH_NOTIFY

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk

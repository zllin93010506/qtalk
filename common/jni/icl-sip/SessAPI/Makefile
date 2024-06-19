###############################################################################
#
# Makefile for common module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = sess_sess.o \
	sess_media.o \
	 sess_payload.o

TARGET = SessAPI
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk

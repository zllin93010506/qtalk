###############################################################################
#
# Makefile for sip module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = base64.o \
	md5c.o \
	sip_cfg.o \
	sip_hdr.o \
	sip_req.o \
	sip_rsp.o \
	sip_tx.o \
	sip_url.o \
	sip_int.o \
	rfc822.o

TARGET = sip
PROGFLAGS = -ansi -std=c99 -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk

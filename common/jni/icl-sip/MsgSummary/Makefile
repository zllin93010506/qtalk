###############################################################################
#
# Makefile 
#
###############################################################################

PROJECT_ROOT = .

CC      = gcc
CXX     = g++
CPP     = gcc
LD      = $(CPP)
AR      = ar
RANLIB  = ranlib
LINT    = lint

MAKE    = make
INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
RM      = rm

CFLAGS   = -fno-builtin -ansi -I../../src -I./

TARGET = msgsummary
PROGFLAGS = 
PROGDEFS =


#OBJECTS = milenage.o \
#	rijndael.o

OBJECTS = msgsummary.o msgcount.o

LIBTARGET = ./lib$(TARGET).a

all: $(LIBTARGET)


$(LIBTARGET): $(OBJECTS) 
	$(RM) -f $@
	@$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(objdir)/%.o: %.c
	$(CC) -c -o $@ $(INC) $(CFLAGS) $(CPPFLAGS) $(DEFS) $<

clean:
	$(RM) -f $(LIBTARGET)  *.o 

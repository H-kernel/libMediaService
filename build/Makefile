#############################################################################
# Makefile for building: gessctrl
#############################################################################
PREFIX          = /usr/local
CC              = gcc
C++             = g++  
AR              = ar 
COMMON_DIR      = ../as_common/
COMMON_LIB      = $(PREFIX)/lib
LICENSE_DIR     = ../extend/
TARGET_LIB      = ./libMediaService.a
TARGET_DLL      = ./libMediaService.so
SRC_DIR         = ./

C++FLAGS  += -pipe -g -w -W -Wall -fPIC -O0 -DENV_LINUX -std=c++98 -fpermissive -D_GNU_SOURCE -DNDEBUG
ARFLAGS   += 

LIBS      = -shared -fPIC -lm -lpthread 

INCPATH   += -I../ -I$(COMMON_DIR)inc/ -I../rtmp -I../rtsp -I../mov/include

CPPFILES  += $(wildcard $(COMMON_DIR)src/*.cpp)
CFILES    += $(wildcard $(COMMON_DIR)src/*.c)
HEADFILES += $(wildcard $(COMMON_DIR)inc/*.h)

CPPFILES  += $(wildcard ../rtsp/*.cpp)
CFILES    += $(wildcard ../rtsp/*.c)
HEADFILES += $(wildcard ../rtsp/*.h)

CPPFILES  += $(wildcard ../rtmp/*.cpp)
CFILES    += $(wildcard ../rtmp/*.c)
HEADFILES += $(wildcard ../rtmp/*.h)

CPPFILES  += $(wildcard ../mov/source/*.cpp)
CFILES    += $(wildcard ../mov/source/*.c)
HEADFILES += $(wildcard ../mov/include/*.h)

CPPFILES  += $(wildcard ../*.cpp)
CFILES    += $(wildcard ../*.c)
HEADFILES += $(wildcard ../*.h)
   
COBJS=$(CFILES:.c=.o)
CPPOBJS=$(CPPFILES:.cpp=.O)


all: $(TARGET_LIB) $(TARGET_DLL)
$(TARGET_DLL): $(CPPOBJS) $(COBJS)
	$(C++) -o $@ $(CPPOBJS) $(COBJS) $(LIBS)

$(TARGET_LIB): $(CPPOBJS) $(COBJS)
	$(AR) rcs $@ $(CPPOBJS) $(COBJS) 

%.o : %.c $(HEADFILES)
	$(CC) -c  $(C++FLAGS) $(INCPATH) $< -o $@

%.O : %.cpp $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@	

clean:
	rm -f $(TARGET_LIB) $(TARGET_DLL)  $(CPPOBJS) $(COBJS)

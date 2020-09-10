#############################################################################
# Makefile for building: gessctrl
#############################################################################
PREFIX          = /usr/local
CC              = gcc 
AR              = ar 
COMMON_DIR      = ./as_common/
COMMON_LIB      = $(PREFIX)/lib
LICENSE_DIR     = ../extend/
TARGET_LIB      = ./libMediaService.a
TARGET_DLL      = ./libMediaService.so
SRC_DIR         = ./

CFLAGS    += -pipe -g -w -fPIC -O0 -DENV_LINUX
ARFLAGS   += 

LIBS      = -shared -fPIC -lm -lpthread 

INCPATH   += -I./ -I$(COMMON_DIR) -I./rtmp -I./rtsp 

CPPFILES  += $(wildcard $(COMMON_DIR)*.cpp)
CFILES    += $(wildcard $(COMMON_DIR)*.c)
HEADFILES += $(wildcard $(COMMON_DIR)*.h)

CPPFILES  += $(wildcard ./rtsp/*.cpp)
CFILES    += $(wildcard ./rtsp/*.c)
HEADFILES += $(wildcard ./rtsp/*.h)

CPPFILES  += $(wildcard ./rtmp/*.cpp)
CFILES    += $(wildcard ./rtmp/*.c)
HEADFILES += $(wildcard ./rtmp/*.h)
   
COBJS=$(CFILES:.c=.o)
CPPOBJS=$(CPPFILES:.cpp=.O)


all: $(TARGET_LIB) $(TARGET_DLL)
$(TARGET_DLL): $(CPPOBJS) $(COBJS)
	$(CC) -o $@ $(CPPOBJS) $(COBJS) $(LIBS)

$(TARGET_LIB): $(CPPOBJS) $(COBJS)
	$(AR) rcs $@ $(CPPOBJS) $(COBJS) 

%.o : %.c $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@

%.O : %.cpp $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@	

clean:
	rm -f $(TARGET) $(CPPOBJS) $(COBJS)

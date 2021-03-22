#############################################################################
# Makefile for building: gessctrl
#############################################################################

C++        = g++ 
COMMON_DIR = ../../as_common/
TARGET     = ./test_rtxp_client

INCPATH   += -I../ -I$(COMMON_DIR)


LIBS      += -L../ -lMediaService -lpthread
LIBS      += -lm -ldl


C++FLAGS  += -pipe -g -Wall -O0 -DENV_LINUX -fPIC


CPPFILES += $(wildcard ./*.cpp)
CFILES += $(wildcard ./*.c)
HEADFILES += $(wildcard ./*.h)
	   
COBJS=$(CFILES:.c=.o)
CPPOBJS=$(CPPFILES:.cpp=.O)


all: $(TARGET)
$(TARGET): $(CPPOBJS) $(COBJS)
	$(C++) $(C++FLAGS) -o $@ $(CPPOBJS) $(COBJS) $(LIBS)
	
%.o : %.c $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@

%.O : %.cpp $(HEADFILES)
	$(C++) -c  $(C++FLAGS) $(INCPATH) $< -o $@	

clean:
	rm -f $(TARGET) $(CPPOBJS) $(COBJS) ./libMediaService.so


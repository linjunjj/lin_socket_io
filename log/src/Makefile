TARGET  := libglog.so
CXX     := g++
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -I. -I ../../
CFLAGS  := -g -Wall -fPIC $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -std=c++11 
SHARE   := -shared -o

#source file
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

.PHONY: everything objs clean veryclean rebuild

everything: $(TARGET)

all: $(TARGET)
	rm -fr *.o
	cp *.h ../
	mv $(TARGET) ../../lib/so

objs: $(OBJS)

rebuild: veryclean everything

clean:
	rm -fr *.o
	rm -rf ../*.h
	rm -rf ../../lib/so/$(TARGET)

veryclean : clean
	rm -fr $(TARGET)
    
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(SHARE) $@ $(OBJS) $(LDFLAGS) $(LIBS)

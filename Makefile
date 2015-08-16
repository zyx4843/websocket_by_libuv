#
# 编译可执行文件, 动态库模板
#

#系统参数
SHELL = /bin/bash
DIR_CUR = $(shell pwd)

CAR = ar
CXX = g++
RM = rm -f


#从目录检索需要编译的.c,.cpp文件
#DIRS =src main

#FIND_FILES_CPP = $(wildcard $(DIR_CUR)/$(dir)/*.cpp)
#FIND_FILES_C = $(wildcard $(DIR_CUR)/$(dir)/*.c)

#SOURCES = $(foreach dir, $(DIRS), $(FIND_FILES_C))
#SOURCES += $(foreach dir, $(DIRS), $(FIND_FILES_CPP))

SOURCES:=$(wildcard *.cpp)

#OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))


#用户自定义编译选项 -D__LINUX64__
CXXFLAGS = -D__LINUX64__

#优化选项 -g 
CFLAGS = 

#加载库选项 -Wl,-O1
LDFLAGS =


#引用头文件
INCLUDES = -I $(DIR_CUR)/src

#引用的静态库,以及静态库目录
DIR_STATIC = 
LIB_STATIC = -Bstatic ./lib/libuv.a

#引用的动态库,以及动态库目录
DIR_DYNAMIC = 
LIB_DYNAMIC = -Bdynamic -lpthread


#整合参数
PARAMS = $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $(DIR_STATIC) $(LIB_STATIC) $(DIR_DYNAMIC) $(LIB_DYNAMIC) $(INCLUDES)


#编译目标, obj,so
TARGET = obj
OBJ_NAME = ./bin/ws
OBJ_PARAM = -o $(OBJ_NAME)

ifeq ($(TARGET), so)
	OBJ_NAME := lib$(OBJ_NAME).so
	OBJ_PARAM = -fpic -shared -o $(OBJ_NAME)
else
	OBJ_PARAM = -o $(OBJ_NAME)
endif


.PHONY:all clean

all:
	@echo "++++++++++++++++ make all ++++++++++++++++"
	@echo "++ DIR_CUR=" $(DIR_CUR)
	@echo "++ CAR=" $(CAR)
	@echo "++ CXX=" $(CXX)
	@echo "++ CFLAGS=" $(CFLAGS)
	@echo "++ CXXFLAGS=" $(CXXFLAGS)
	@echo "++ LDFLAGS=" $(LDFLAGS)
	@echo "++ INCLUDES=" $(INCLUDES)
	@echo "++ DIR_STATIC=" $(DIR_STATIC)
	@echo "++ LIB_STATIC=" $(LIB_STATIC)
	@echo "++ DIR_DYNAMIC=" $(DIR_DYNAMIC)
	@echo "++ LIB_DYNAMIC=" $(LIB_DYNAMIC)
	@echo "++ OBJ_PARAM=" $(OBJ_PARAM)
	@echo "++ PARAMS=" $(PARAMS)
	@echo "++ SOURCES=" $(notdir $(SOURCES))
	@echo "++++++++++++++++ cxx start ++++++++++++++++"
	
	$(CXX) $(OBJ_PARAM) $(SOURCES) $(PARAMS) 
	
clean:
	@echo "++++++++++++++++ make clean ++++++++++++++++"
	@echo "++ RM=" $(RM)
	
	$(RM) $(OBJ_NAME)

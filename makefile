######################################################################
#                         Makefile                                   #
######################################################################
CC          := g++
SRC_DIR     := ./src/xHttpd.cpp ./src/xevHtpdFrontend.cpp ./src/xevHtpdBackend.cpp ./src/xHttpCache.cpp 
SRC_DIR     += ./src/xExpireManager.cpp ./src/xDataBase.cpp ./src/xConfig.cpp ./src/xRedisServer.cpp ./src/xEtagManager.cpp
SRC_DIR     += ./common/jsoncpp.cpp ./common/xIniFile.cpp ./common/xLog.cpp ./common/xRedisServerLib.cpp ./common/MurmurHash2.cpp
SRC_DIR     += ./common/util.cpp ./common/sds.cpp ./common/sorted_set.cpp ./common/xevHtpdBase.cpp
SRC_DIR     += ./common/Exception.cpp ./common/Field.cpp ./common/Parser.cpp 
INC_DIR     := ./ ./common ./include ./include/evhtp
LIB_DIR     := ./lib /usr/local/lib64  /usr/local/lib

INSTALL_PATH := .
# /usr/local/ssl/lib/
SRC_FILES   := $(wildcard $(SRC_DIR))
OBJ_FILES   := $(SRC_FILES:.cpp=.o)
LD_LIBS     :=  ssl crypto rt pthread dl
AR_LIBS     :=  evhtp event event_pthreads event_openssl rocksdb  snappy bz2 z 
STATIC_LIBS := -Wl,-Bstatic $(addprefix -l,$(AR_LIBS)) -Wl,-Bdynamic

APP_NAME    := xhttpcache
APP_SUFFIX  := 
APP_TARGET  := $(APP_NAME)$(APP_SUFFIX)

.PHONY: all clean

LDFLAGS     := $(addprefix -L,$(LIB_DIR)) $(STATIC_LIBS)  $(addprefix -l,$(LD_LIBS)) -Wl,-export-dynamic  
CFLAGS      := $(addprefix -I,$(INC_DIR))  -D_REENTRANT 
CFLAGS      += -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
CFLAGS      += -std=c++11 -D__STDC_FORMAT_MACROS


all: $(APP_TARGET);

$(APP_TARGET): $(OBJ_FILES)
	$(CC) -o $(APP_TARGET) $(CFLAGS) $(OBJ_FILES) $(LDFLAGS)
	@echo *********Build $(APP_TARGET:.a=.so) Successful*********


%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

install:
	@echo install...

clean:
	@echo clean...
	@rm -rf $(OBJ_FILES)
	@rm -rf *.orig


    
    


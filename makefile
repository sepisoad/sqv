IGNORE = .ignore
BUILD_DIR = $(IGNORE)/build

HOST_BIN = sqv
PLUGIN_SO = live.so

C_DEPS = -Ideps/c -I/usr/local/include
SRC_DIR = src

LFS_DIR = $(C_DEPS)/lfs
STB_DIR = $(C_DEPS)/stb
OBJ_DIR = $(BUILD_DIR)

HOST_OBJ = $(BUILD_DIR)/host.o
PLUGIN_OBJ = $(BUILD_DIR)/live.o
OBJS = $(HOST_OBJ)
HOST = $(BUILD_DIR)/$(HOST_BIN)
PLUGIN = $(BUILD_DIR)/$(PLUGIN_SO)


CC = gcc-14
CC_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
CC_FLAGS = $(C_DEPS) -std=c23 -DRAYLIB_LIBTYPE=SHARED -DPLATFORM=PLATFORM_DESKTOP_RGFW

DEBUG ?= 1
ifeq ($(DEBUG), 1)
		CC_FLAGS += -g -O0 -DDEBUG
else
		CC_FLAGS += -O2 -DNDEBUG
		MESA_DEBUG = 1
endif

all: $(HOST) $(PLUGIN)
plugin: $(PLUGIN)

# build live lib (hot reloadable shared lib)
$(PLUGIN): $(PLUGIN_OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -shared -o $(PLUGIN) $(PLUGIN_OBJ)

# build live object (hot reloadable object)
$(PLUGIN_OBJ): $(SRC_DIR)/plugin.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -c $(SRC_DIR)/plugin.c -o $(PLUGIN_OBJ)

# generate binary from object files
$(HOST): $(HOST_OBJ)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -o $(HOST) $(OBJS)

# build host (main binary)
$(HOST_OBJ): $(SRC_DIR)/main.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -c $(SRC_DIR)/main.c -o $(HOST_OBJ)


# clean build dir
clean:
	rm -rf $(BUILD_DIR)

clean_plugin:
	rm -rf $(PLUGIN)

# force targets to run from scratch on invocation
.PHONY: clean 
.PHONY: reaload

### ===============================================
### DEBUG HOSTS
### ===============================================
run:
	@$(HOST)
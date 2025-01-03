IGNORE = .ignore
BUILD_DIR = $(IGNORE)/build

HOST_BIN = sqv
PLUGIN_SO = plugin.so

C_DEPS = -Ideps/c -I/usr/local/include
SRC_DIR = src

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

# Source files
HOST_SOURCES = main.c hotreload.c
PLUGIN_SOURCES = plugin.c

# Object files
HOST_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(HOST_SOURCES))
PLUGIN_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(PLUGIN_SOURCES))

# Targets
HOST = $(BUILD_DIR)/$(HOST_BIN)
PLUGIN = $(BUILD_DIR)/$(PLUGIN_SO)

# Rules
all: $(HOST) $(PLUGIN)
plugin: $(PLUGIN)

# Build plugin shared library
$(PLUGIN): $(PLUGIN_OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -shared -o $@ $^

# Build host binary
$(HOST): $(HOST_OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) $(CC_LIBS) -o $@ $^

# Generic rule to build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -c $< -o $@

# Clean build dir
clean:
	rm -rf $(BUILD_DIR)

clean_plugin:
	rm -rf $(PLUGIN)

# Force targets to run from scratch on invocation
.PHONY: clean clean_plugin plugin all


### ===============================================
### DEBUG HOSTS
### ===============================================
run:
	@$(HOST)
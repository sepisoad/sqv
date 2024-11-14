IGNORE = .ignore
BUILD_DIR = $(IGNORE)/build
BIN = sqv
C_DEPS = deps/c
SRC_DIR = src
LFS_DIR = $(C_DEPS)/lfs
STB_DIR = $(C_DEPS)/stb
OBJ_DIR = $(BUILD_DIR)

SRC_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
LFS_OBJS = $(patsubst $(LFS_DIR)/%.c, $(BUILD_DIR)/lfs_%.o, $(wildcard $(LFS_DIR)/*.c))
STB_OBJS = $(patsubst $(STB_DIR)/%.c, $(BUILD_DIR)/stb_%.o, $(wildcard $(STB_DIR)/*.c))
OBJS = $(SRC_OBJS) $(LFS_OBJS) $(STB_OBJS)
TARGET = $(BUILD_DIR)/$(BIN)


CC = gcc-14
C_LIBS = -llua5.4 -lm
C_FLAGS = $(DEBUG_FLAGS) -I$(C_DEPS) -std=c23

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    C_FLAGS += -g -O0 -DDEBUG
else
    C_FLAGS += -O2 -DNDEBUG
		MESA_DEBUG = 1
endif

all: $(TARGET)

# generate binary from object files
$(TARGET): $(OBJS)
	$(CC) $(C_FLAGS) $(C_LIBS) -o $@ $^

# build sqv c file(s)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(C_LIBS) -c $< -o $@

# build lfs lua c binding
$(BUILD_DIR)/lfs_%.o: $(LFS_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(C_LIBS) -c $< -o $@

# build stb lua c binding
$(BUILD_DIR)/stb_%.o: $(STB_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(C_LIBS) -c $< -o $@

# clean build dir
clean:
	rm -rf $(BUILD_DIR)

# force targets to run from scratch on invocation
.PHONY: all clean

### ===============================================
### DEBUG TARGETS
### ===============================================
run:
	@$(TARGET)
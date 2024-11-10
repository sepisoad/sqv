CC = gcc
IGNORE = .ignore
BUILDDIR = $(IGNORE)/build
BIN = $(BUILDDIR)/sqv
CFLAGS = 
# LIBS = -lglfw3 -lGL -lGLU -lGLEW -lrt -lm -ldl
LIBS = -lSDL2 -lGL -lm -lGLU -lGLEW
SRC_DIR = src
OBJ_DIR = $(BUILDDIR)

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
		MESA_DEBUG = 1
endif

all: build

clean:
	rm -rf $(IGNORE)

builddir:
	mkdir -p $(BUILDDIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build: builddir $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) -o $(BIN) $(LIBS)

### ===============================================
### DEBUG TARGETS
### ===============================================
run:
	@$(BIN)
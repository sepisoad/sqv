CC = gcc-14
IGNORE = .ignore
BUILDDIR = $(IGNORE)/build
BIN = $(BUILDDIR)/sqv
LIBS = -lSDL2 -lGL -lm -lGLU -lGLEW
SRC_DIR = src
OBJ_DIR = $(BUILDDIR)

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

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
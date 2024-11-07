CC = gcc-14
IGNORE = .ignore
BUILDDIR = $(IGNORE)/build
BIN = $(BUILDDIR)/sqv

all: build

clean:
	rm -rf $(IGNORE)

builddir:
	mkdir -p $(BUILDDIR)

build: builddir
	$(CC) src/main.c -o $(BIN) 

### ===============================================
### DEBUG TARGETS
### ===============================================
run:
	@$(BIN)
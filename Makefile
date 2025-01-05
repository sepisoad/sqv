# GNU Make workspace makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

ifeq ($(config),debug)
  Host_config = debug
  Plugin_config = debug
endif
ifeq ($(config),release)
  Host_config = release
  Plugin_config = release
endif

PROJECTS := Host Plugin

.PHONY: all clean help $(PROJECTS) 

all: $(PROJECTS)

Host:
ifneq (,$(Host_config))
	@echo "==== Building Host ($(Host_config)) ===="
	@${MAKE} --no-print-directory -C . -f Host.make config=$(Host_config)
endif

Plugin:
ifneq (,$(Plugin_config))
	@echo "==== Building Plugin ($(Plugin_config)) ===="
	@${MAKE} --no-print-directory -C . -f Plugin.make config=$(Plugin_config)
endif

clean:
	@${MAKE} --no-print-directory -C . -f Host.make clean
	@${MAKE} --no-print-directory -C . -f Plugin.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "  debug"
	@echo "  release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   Host"
	@echo "   Plugin"
	@echo ""
	@echo "For more information, see https://github.com/premake/premake-core/wiki"
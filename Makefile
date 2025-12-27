# X68000 OpenAI Chat System - Makefile
# This makefile is designed for use with msys, cygwin, or similar Unix-compatible environments.
# Requires xdev68k: https://github.com/yosshin4004/xdev68k

# Verify XDEV68K_DIR is defined
ifndef XDEV68K_DIR
	$(error ERROR : XDEV68K_DIR is not defined. Set it to the xdev68k installation path.)
endif

# Clear default suffixes
.SUFFIXES:

# Target CPU
CPU = 68000

# Command shortcuts
ATOMIC = perl ${XDEV68K_DIR}/util/atomic.pl
CC = ${XDEV68K_DIR}/m68k-toolchain/bin/m68k-elf-gcc
GAS2HAS = perl ${XDEV68K_DIR}/util/x68k_gas2has.pl -cpu $(CPU) -inc doscall.inc -inc iocscall.inc
RUN68 = $(ATOMIC) ${XDEV68K_DIR}/run68/run68
HAS = $(RUN68) ${XDEV68K_DIR}/x68k_bin/HAS060.X
HLK = $(RUN68) ${XDEV68K_DIR}/x68k_bin/hlk301.x

# Include paths
INCLUDE_FLAGS = -I${XDEV68K_DIR}/include/xc -I${XDEV68K_DIR}/include/xdev68k -Isrc/libchat

# Compiler flags
COMMON_FLAGS = -m$(CPU) -Os $(INCLUDE_FLAGS)
CFLAGS = $(COMMON_FLAGS) -Wno-builtin-declaration-mismatch -fcall-used-d2 -fcall-used-a2 \
         -finput-charset=cp932 -fexec-charset=cp932 -fverbose-asm

# X68000 system libraries
LIBS = \
	${XDEV68K_DIR}/lib/xc/BASLIB.L \
	${XDEV68K_DIR}/lib/xc/CLIB.L \
	${XDEV68K_DIR}/lib/xc/DOSLIB.L \
	${XDEV68K_DIR}/lib/xc/IOCSLIB.L \
	${XDEV68K_DIR}/lib/xc/FLOATFNC.L \
	${XDEV68K_DIR}/lib/m68k_elf/m$(CPU)/libgcc.a

# Directories
SRCDIR = src
BUILDDIR = build/m$(CPU)

# Library source files
LIBCHAT_SRCS = \
	$(SRCDIR)/libchat/chat.c \
	$(SRCDIR)/libchat/serial.c \
	$(SRCDIR)/libchat/protocol.c

# CLI source files
CLI_SRCS = $(SRCDIR)/cli/main.c

# Object files
LIBCHAT_OBJS = $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(LIBCHAT_SRCS)))
CLI_OBJS = $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(CLI_SRCS)))

# All object files for CLI (library objects linked directly)
ALL_CLI_OBJS = $(CLI_OBJS) $(LIBCHAT_OBJS)

# Dependency files
DEPS = $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.d,$(LIBCHAT_SRCS) $(CLI_SRCS)))

# Target files
CLI_TARGET = $(BUILDDIR)/CHAT.X

# HLK link list
HLK_LINK_LIST = $(BUILDDIR)/_lk_list.tmp

# Default target
all: $(CLI_TARGET)

# Include dependencies
-include $(DEPS)

# Clean build artifacts
clean:
	rm -rf build

# Build CLI executable
# HLK cannot handle long paths well, so we copy libs to build dir
$(CLI_TARGET): $(ALL_CLI_OBJS)
	@mkdir -p $(BUILDDIR)
	rm -f $(HLK_LINK_LIST)
	@for FILENAME in $(ALL_CLI_OBJS); do \
		echo $$FILENAME >> $(HLK_LINK_LIST); \
	done
	@for FILENAME in $(LIBS); do \
		cp $$FILENAME $(BUILDDIR)/`basename $$FILENAME`; \
		echo $(BUILDDIR)/`basename $$FILENAME` >> $(HLK_LINK_LIST); \
	done
	$(HLK) -i $(HLK_LINK_LIST) -o $@
	@echo "Built: $@"

# Compile C source files
$(BUILDDIR)/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) -S $(CFLAGS) -o $(BUILDDIR)/$*.m68k-gas.s -MT $@ -MD -MP -MF $(BUILDDIR)/$*.d $<
	$(GAS2HAS) -i $(BUILDDIR)/$*.m68k-gas.s -o $(BUILDDIR)/$*.s
	rm -f $(BUILDDIR)/$*.m68k-gas.s
	$(HAS) -e -u -w0 $(INCLUDE_FLAGS) $(BUILDDIR)/$*.s -o $@

.PHONY: all clean

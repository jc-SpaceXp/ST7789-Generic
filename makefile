# Specifiy toolset, must be compatible with your target app
# e.g. build library with gcc then you must also build your app with gcc
TOOLSET ?= arm-none-eabi-
AR := $(TOOLSET)ar
CC := $(TOOLSET)gcc
AS := $(TOOLSET)as
GDB := $(TOOLSET)gdb
SIZE := $(TOOLSET)size
OBJCOPY := $(TOOLSET)objcopy


SRCDIR = src
INCDIR = inc
LIBDIR = lib
OBJDIR = .obj
DEPDIR = .deps

COMMON_CFLAGS = -Wall -Wextra -std=c11 -g3 -Os

# Flags specific to your CC toolset (should mirror your main build)
# below is a STM32G4xx example
CPUFLAGS ?= -mcpu=cortex-m4 -mthumb
FPUFLAGS ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16

AFLAGS := -D --warn $(CPUFLAGS) -g
CPPFLAGS := -I $(INCDIR)
CFLAGS := $(CPUFLAGS) $(FPUFLAGS) $(COMMON_CFLAGS) -ffunction-sections -fdata-sections
DEPFLAGS = -MT $@ -MMD -MP -MF $(@:$(OBJDIR)/%.o=$(DEPDIR)/%.d)

LIBSRCS := $(wildcard $(SRCDIR)/*.c)
LIBSRCOBJS := $(LIBSRCS:%.c=$(OBJDIR)/%.o)
LIBSRCDEPS := $(LIBSRCS:%.c=$(DEPDIR)/%.d)

LIBTARGET = lib_st7789_generic
STM32G4EXAMPLE = stm32g4_main
SPITESTTARGET = spi_tests
ST7789TESTTARGET = st7789_tests

TESTCC := gcc
TESTSIZE := size

TESTDIR = tests
MOCKLIBDIR = lib/fff
TESTLIBDIR = lib/greatest
TESTOBJDIR := $(OBJDIR)/$(TESTDIR)
TESTCPPFLAGS := -I $(INCDIR) -I $(TESTLIBDIR) -I $(TESTDIR) -I $(MOCKLIBDIR)
TESTCFLAGS := $(COMMON_CFLAGS) $(CMSIS_CPPFLAGS)

SPI_TESTSRCS := $(TESTDIR)/spi_suite.c $(TESTDIR)/spi_main.c
SPI_TESTSRCS += $(SRCDIR)/spi.c
SPI_TESTOBJS := $(SPI_TESTSRCS:%.c=$(TESTOBJDIR)/%.o)

ST7789_TESTSRCS := $(TESTDIR)/st7789_suite.c $(TESTDIR)/st7789_main.c
ST7789_TESTSRCS += $(SRCDIR)/st7789.c
ST7789_TESTOBJS := $(ST7789_TESTSRCS:%.c=$(TESTOBJDIR)/%.o)


.PHONY: all examples clean tests srcdepdir test_modules_git_update
all: $(LIBTARGET).a
examples: $(STM32G4EXAMPLE).elf $(STM32G4EXAMPLE).bin
tests: $(SPITESTTARGET).elf $(ST7789TESTTARGET).elf

$(LIBTARGET).a: $(LIBSRCOBJS)
	@echo "Creating static library"
	$(AR) rcs $@ $^
	$(SIZE) $@

$(OBJDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c | srcdepdir
	@echo "Creating library objects"
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

srcdepdir :
	@mkdir -p $(DEPDIR)/$(SRCDIR)

$(SRCDEPS):

test_modules_git_update:
	@echo "Initializing/updating greatest submodule"
	git submodule update --init --remote $(LIBDIR)/greatest $(LIBDIR)/fff

# Unit test builds
$(SPITESTTARGET).elf: $(SPI_TESTOBJS) | test_modules_git_update
	@echo "Linking test objects"
	$(TESTCC) $(TESTLDFLAGS) $(TESTLDLIBS) $^ -o $@
	$(TESTSIZE) $@

$(ST7789TESTTARGET).elf: $(ST7789_TESTOBJS) | test_modules_git_update
	@echo "Linking test objects"
	$(TESTCC) $(TESTLDFLAGS) $(TESTLDLIBS) $^ -o $@
	$(TESTSIZE) $@

$(TESTOBJDIR)/%.o: %.c
	@echo "Creating test objects"
	@mkdir -p $(@D)
	$(TESTCC) $(TESTCPPFLAGS) $(TESTCFLAGS) -c $< -o $@


clean:
	@echo "Cleaning build"
	-$(RM) $(LIBTARGET).a $(SPITESTTARGET).elf $(ST7789TESTTARGET).elf $(STM32G4EXAMPLE).elf
	-$(RM) -rf $(OBJDIR) $(DEPDIR)

-include $(wildcard $(SRCDEPS))

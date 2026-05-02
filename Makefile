# gcc options
# https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# ld options
# https://sourceware.org/binutils/docs/ld/Options.html

# right now this will be linux only but with cross compiler capabilities


TARGET := rfsweep
#INCLUDEE := hackrf.h

# COPYOVER_LINUX   := src/process.py src/transmit.sh  src/run.sh  src/fastrun.sh
# COPYOVER_WINDOWS := src/process.py src/transmit.bat src/run.bat src/fastrun.bat
# COPYOVER_WINDOWS := $(COPYOVER_WINDOWS) lib/cygwin/bin/cygwin1.dll lib/libhackrf/windows/hackrf-tools

#COPYOVER_LINUX   := src/process.py src/linux/* LICENSE README.md
#COPYOVER_WINDOWS := src/process.py src/windows/*
#COPYOVER_WINDOWS := $(COPYOVER_WINDOWS) lib/cygwin/bin/cygwin1.dll lib/libhackrf/windows/hackrf-tools
COPYOVER_WINDOWS := src/windows/* lib/libhackrf/windows/hackrf-tools
COPYOVER_LINUX   := src/linux/*
COPYOVER_RASPI   := src/raspi/*
COPYOVER_ALL	 :=

TMPDIR := tmp/
SRCDIR := src/rfsweep/
INCDIR := inc/
BINDIR := bin/
# this makefile expects libraries to use the same directory structure and names as specified above
LIBDIR := lib


WINDOWS_LIBS :=
LINUX_LIBS   :=
# TODO: consider renaming "wasm" to "web"
WASM_LIBS    :=



CC := gcc
#CC_WINDOWS := x86_64-w64-mingw32-gcc-posix
# CC_WINDOWS := x86_64-w64-mingw32-gcc
#CC_WINDOWS := wine ./lib/cygwin/gcc.exe
CC_WINDOWS := gcc
# NOTE: -std=c23 is strict c23 compliance, whereas -std=gnu23 includes gnu specific features
# Here are the extensions
# https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html
CFLAGS := -fvisibility=internal -D__ENABLE_SYSMESSAGES__
# -llibhackrf.so -lhackrf
#LFLAGS := -L./lib/kissfft/ -lusb-1.0 -Wl,-Bstatic -lkissfft-float -Wl,-Bdynamic -lm
LFLAGS := -Wl,-Bstatic -Wl,-Bdynamic -lm
BFLAGS := -Wall -Wextra
#LIBS   :=


DB := gdb
DBFLAGS :=

AR := ar
ARFLAGS := rcs

PP := m4


FAST_CFLAGS := -D__DEBUG__ -D__debug__
FAST_LFLAGS := 
FAST_BFLAGS := -g -O0 

# -fsanitize=thread
DEBUG_CFLAGS := -D__DEBUG__ -D__debug__
DEBUG_LFLAGS := 
DEBUG_BFLAGS := -g -Og -fsanitize=address,undefined -fno-omit-frame-pointer -latomic -static-libasan

RELEASE_CFLAGS :=
RELEASE_LFLAGS := -s
RELEASE_BFLAGS := -Os -g0
# RELEASE_BFLAGS := -Os -g


STATIC_CFLAGS := -D__STATIC_LIB__
STATIC_LFLAGS := 
STATIC_BFLAGS := 

DYNAMIC_CFLAGS := -D__DYNAMIC_LIB__ -fpic
DYNAMIC_LFLAGS := 
DYNAMIC_BFLAGS := 


WINDOWS_CFLAGS := -Ilib/cygwin/include -Ilib/toolkit/inc -D__WIN32__ -std=gnu2x
WINDOWS_LFLAGS := -Llib/cygwin/lib -lcygwin -lpthread
WINDOWS_BFLAGS :=

LINUX_CFLAGS := -D__LINUX__ -I/usr/include/libusb-1.0 -std=gnu2x
LINUX_LFLAGS := -lusb-1.0
LINUX_BFLAGS := 
#LINUX_LIBS   :=

WASM_CFLAGS := -D__WEBASM__
WASM_LFLAGS :=
WASM_BFLAGS :=



DEFAULT := release
WHITELIST := all fast release debug static dynamic 
WHITELIST := $(WHITELIST) static_fast static_debug static_release
WHITELIST := $(WHITELIST) dynamic_fast dynamic_debug dynamic_release
WHITELIST := $(WHITELIST) preproc preproc_debug




# ============================================
# ============================================



# so the first word is the goal, and subsiquent words are modifiers
# the goal is basically optimization target, but subsiquent goal can be static
GOAL := $(firstword $(MAKECMDGOALS))
GOAL := $(if $(GOAL),$(GOAL),$(DEFAULT))
# should grab all sources relative to makefile.
# colon equal (:=) removed, so as to grab all generated c files as well
SRCS = $(shell /usr/bin/find -L $(SRCDIR) -type f -name "*.c")
INCS = $(shell /usr/bin/find -L $(INCDIR) -type f -name "*.h")
# $(info $(SRCS))
ifneq ($(filter $(GOAL),$(WHITELIST)),)
	OBJS = $(SRCS:%.c=$(TMPDIR)/$(GOAL)/%.o)
	DEPS = $(SRCS:%.c=$(TMPDIR)/$(GOAL)/%.d)
	PREP = $(SRCS:%.c=$(TMPDIR)/$(GOAL)/%.pp)
endif

#BINGOAL :=  $(subst dynamic_,,$(subst static_,,$(GOAL)))
#BINTARG := $(BINDIR)/$(BINGOAL)/$(TARGET)
BINTARG := $(BINDIR)/$(TARGET)
LIBBINTARG := $(BINDIR)/lib$(TARGET)
#INCTARG := $(BINDIR)/$(INCLUDEE)

LIBDIRS := $(shell ls -d $(LIBDIR)/)
LIBINCS := $(foreach path,$(LIBDIRS),-I$(path)$(INCDIR)/)
LIBBINS := $(foreach path,$(LIBDIRS),-L$(path)$(BINDIR)/)
CFLAGS  := $(CFLAGS) $(LIBINCS)
LFLAGS  := $(LFLAGS) $(LIBBINS)


ifndef WINDOWS
ifndef LINUX
ifeq ($(OS),Windows_NT) 
	#$(error Winbows not yet implemented)
	WINDOWS := WINDOWS
else
	OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
	ifeq ($(OS),Linux)
		#BLFLAGS += -target *-*-linux-gnu
		#MAKEOSFILE := make/linux.mk
		IS_ARM := $(shell echo | $(CC) -dM -E - | grep -E '__arm__|__aarch64__' >/dev/null && echo yes)
		ifneq ($(IS_ARM),yes)
			LINUX := LINUX
		else
			RASPI := RASPI
			LFLAGS += -lpigpio
		endif
	else
		$(error Incompatable operating system)
	endif
endif
endif
endif


#==========================================
#==========================================


all: $(DEFAULT)
fast: _fast $(BINTARG)
debug: _debug $(BINTARG)
release: _release $(BINTARG)

preproc: _release $(PREP)
preproc_debug: _debug $(PREP)

static: static_$(DEFAULT)
static_fast: _static _fast $(LIBBINTARG).a
static_debug: _static _debug $(LIBBINTARG).a
static_release: _static _release $(LIBBINTARG).a


-include $(DEPS)


# -sGL_DEBUG=1
_fast:
	$(eval CFLAGS += $(FAST_CFLAGS))
	$(eval LFLAGS += $(FAST_LFLAGS))
	$(eval BFLAGS += $(FAST_BFLAGS))

# -sGL_DEBUG=1
_debug:
	$(eval CFLAGS += $(DEBUG_CFLAGS))
	$(eval LFLAGS += $(DEBUG_LFLAGS))
	$(eval BFLAGS += $(DEBUG_BFLAGS))

# -Oz  ## smaller than -Os, but also slower
#  -sMODULARIZE
_release:
	$(eval CFLAGS += $(RELEASE_CFLAGS))
	$(eval LFLAGS += $(RELEASE_LFLAGS))
	$(eval BFLAGS += $(RELEASE_BFLAGS))	

_static:
	$(eval CFLAGS += $(STATIC_CFLAGS))
	$(eval LFLAGS += $(STATIC_LFLAGS))
	$(eval BFLAGS += $(STATIC_BFLAGS))

_dynamic:
	$(eval CFLAGS += $(DYNAMIC_CFLAGS))
	$(eval LFLAGS += $(DYNAMIC_LFLAGS))
	$(eval BFLAGS += $(DYNAMIC_BFLAGS))



#=========================================#
#      LINUX                              #
#=========================================#
ifdef LINUX


COPYOVER := $(COPYOVER_LINUX) $(COPYOVER_ALL)


CFLAGS += $(LINUX_CFLAGS)
LFLAGS += $(LINUX_LFLAGS)
BFLAGS += $(LINUX_BFLAGS)


# dynamic: dynamic_$(DEFAULT)
# dynamic_fast: _dynamic _fast $(LIBBINTARG).so
# dynamic_debug: _dynamic _debug $(LIBBINTARG).so
# dynamic_release: _dynamic _release $(LIBBINTARG).so


endif
#==========================================
#==========================================



ifdef RASPI


COPYOVER := $(COPYOVER_RASPI) $(COPYOVER_ALL)


CFLAGS += $(LINUX_CFLAGS)
LFLAGS += $(LINUX_LFLAGS)
BFLAGS += $(LINUX_BFLAGS)

endif




ifdef WINDOWS

COPYOVER := $(COPYOVER_WINDOWS) $(COPYOVER_ALL)

CFLAGS += $(WINDOWS_CFLAGS)
LFLAGS += $(WINDOWS_LFLAGS)
BFLAGS += $(WINDOWS_BFLAGS)

CC := $(CC_WINDOWS)

endif



#$(BINTARG): FORCE
# $(BINTARG): $(OBJS)
# 	@mkdir -p $(dir $@)
# 	$(CC) $(BFLAGS) -o $@ $^ -L$(LIBDIR) $(LFLAGS)
# 	cp src/process.py src/run.bat bin/

$(BINTARG): $(OBJS) FORCE Makefile
	@mkdir -p $(dir $@)
	$(CC) $(BFLAGS) -o $@ $(OBJS) -L$(LIBDIR) $(LFLAGS)
	cp -Lr --preserve=mode $(COPYOVER) bin/

# $(LIBBINTARG).a: $(OBJS)
# 	@mkdir -p $(dir $@)
# 	$(AR) $(ARFLAGS) $@ $^
# 
# $(LIBBINTARG).so: $(OBJS)
# 	@mkdir -p $(dir $@)
# 	$(CC) $(BFLAGS) -o $@ $^ -L$(LIBDIR) $(LFLAGS)

# Lets just implement header compilation and consolidation later
#$(INCTARG): $(HEADERS)
#	$(PP) $(INCTARG)
#	$(CC) -I$(INCDIR) $(BFLAGS) $(CFLAGS) -E -P -fpreprocessed include/bitwin.h -o bin/bitwin.pp.h
#	# remember to create a compiled version of the header

$(TMPDIR)/$(GOAL)/%.o: %.c $(TMPDIR)/$(GOAL)/%.d Makefile
	@mkdir -p $(dir $@)
	$(CC) -I$(INCDIR)  $(BFLAGS) $(CFLAGS) -c $< -o $@

$(TMPDIR)/$(GOAL)/%.d: %.c Makefile
	@mkdir -p $(dir $@)
	@$(CC) -I$(INCDIR) $(BFLAGS) $(CFLAGS) -MM -MT $(patsubst %.d,%.o,$@) -MF $@ $<

$(TMPDIR)/$(GOAL)/%.pp: %.c
	@mkdir -p $(dir $@)
	$(CC) -E -I$(INCDIR)  $(BFLAGS) $(CFLAGS) -c $< > $@


clean:
	rm -rf $(TMPDIR)
	rm -rf $(BINDIR)


#run:
#	sudo ./$(BINTARG)
#	sudo chrt -f 50 ./$(BINTARG)


gdb:
	gdb -x script.gdb --args ./$(BINTARG) test


analyze:
	readelf -a $(BINTARG)


test:
	$(BINDIR)/$(TARGET) test --defaults


server:
	sudo $(BINDIR)/$(TARGET) server



FORCE:



# Example rule behavior
#
# rule: dependancies
#	:
#
# I don't remember what this does, but it is useful, because rules without it behave differently






#.PHONY: all fast debug release static dynamic preprocess
.PHONY: all fast debug release static dynamic preprocess
.PHONY: static_fast static_debug static_release dynamic_fast dynamic_debug dynamic_release
.PHONY: clean test run shaders gdb analyze server FORCE
.PHONY: _build _fast _debug _release _dynamic _static _optimize

uname_S	:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_R	:= $(shell sh -c 'uname -r 2>/dev/null || echo not')

# External programs
CC	?= gcc
LD	:= $(CC)
AR	:= ar
XXD	?= xxd

# Set up source directory for GNU Make
srcdir		:= $(CURDIR)
VPATH		:= $(srcdir)

EXTRA_WARNINGS := -Wcast-align
EXTRA_WARNINGS += -Wformat
EXTRA_WARNINGS += -Wformat-security
EXTRA_WARNINGS += -Wformat-y2k
EXTRA_WARNINGS += -Wshadow
EXTRA_WARNINGS += -Winit-self
EXTRA_WARNINGS += -Wredundant-decls
EXTRA_WARNINGS += -Wswitch-default
EXTRA_WARNINGS += -Wswitch-enum
EXTRA_WARNINGS += -Wno-system-headers
EXTRA_WARNINGS += -Wundef
EXTRA_WARNINGS += -Wwrite-strings
EXTRA_WARNINGS += -Wbad-function-cast
EXTRA_WARNINGS += -Wmissing-declarations
EXTRA_WARNINGS += -Wmissing-prototypes
EXTRA_WARNINGS += -Wnested-externs
EXTRA_WARNINGS += -Wold-style-definition
EXTRA_WARNINGS += -Wstrict-prototypes
EXTRA_WARNINGS += -Wdeclaration-after-statement

# Compile flags
CFLAGS		:= -I$(srcdir)/include -Wall $(EXTRA_WARNINGS) -g -O3 -std=gnu99

# Output to current directory by default
O =

# Make the build silent by default
V =
ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

# Project files
PROGRAMS := trade 

DEFINES =
INCLUDES =
EXTRA_LIBS =

ifeq ($(uname_S),Linux)
	DEFINES += -D_GNU_SOURCE
endif

trade_EXTRA_DEPS += builtin-client.o
trade_EXTRA_DEPS += builtin-server.o
trade_EXTRA_DEPS += die.o
trade_EXTRA_DEPS += trade.o

trade_EXTRA_DEPS += $(COMPAT_DEPS)

CFLAGS += $(DEFINES)
CFLAGS += $(INCLUDES)

DEPS		:= $(patsubst %.o,%.d,$(OBJS))

LIB_FILE := libtrading.a

LIBS := $(LIB_FILE)

LIB_OBJS	+= boe_message.o
LIB_OBJS	+= buffer.o
LIB_OBJS	+= fix_message.o
LIB_OBJS	+= fix_session.o
LIB_OBJS	+= itch4_message.o
LIB_OBJS	+= mbt_quote_message.o
LIB_OBJS	+= soupbin3_session.o
LIB_OBJS	+= read-write.o

LIB_DEPS	:= $(patsubst %.o,%.d,$(LIB_OBJS))

TEST_PROGRAM	:= test-trade
TEST_SUITE_H	:= test/test-suite.h
TEST_RUNNER_C	:= test/test-runner.c
TEST_RUNNER_OBJ := test/test-runner.o

TEST_OBJS += test/boe-test.o
TEST_OBJS += test/harness.o
TEST_OBJS += test/mbt_quote_message-test.o
TEST_OBJS += test/unparse-test.o

TEST_SRC	:= $(patsubst %.o,%.c,$(TEST_OBJS))
TEST_DEPS	:= $(patsubst %.o,%.d,$(TEST_OBJS))

TEST_LIBS := $(LIBS)

# Targets
all: sub-make
.DEFAULT: all
.PHONY: all

ifneq ($(O),)
sub-make: $(O) $(FORCE)
	$(Q) $(MAKE) --no-print-directory -C $(O) -f ../Makefile srcdir=$(CURDIR) _all
else
sub-make: _all
endif

_all: $(LIB_FILE) $(PROGRAMS)
.PHONY: _all

$(O):
ifneq ($(O),)
	$(E) "  MKDIR   " $@
	$(Q) mkdir -p $(O)
endif

%.d: %.c
	$(Q) $(CC) -M -MT $(patsubst %.d,%.o,$@) $(CFLAGS) $< -o $@

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@


$(foreach p,$(PROGRAMS),$(eval $(p): $($(p)_EXTRA_DEPS) $(LIBS)))
$(PROGRAMS): % : %.o
	$(E) "  LINK    " $@
	$(Q) $(LD) $(LDFLAGS) -o $@ $^ $($@_EXTRA_OBJS)

$(LIB_FILE): $(LIB_DEPS) $(LIB_OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(LIB_OBJS)

test: $(TEST_PROGRAM)
	$(E) "  TEST"
	$(Q) ./$(TEST_PROGRAM)
.PHONY: test

BOE_TEST_DATA += test/protocol/boe/login-request-message.bin
BOE_TEST_DATA += test/protocol/boe/logout-request-message.bin
BOE_TEST_DATA += test/protocol/boe/client-heartbeat-message.bin
BOE_TEST_DATA += test/protocol/boe/login-response-message.bin
BOE_TEST_DATA += test/protocol/boe/logout-message.bin
BOE_TEST_DATA += test/protocol/boe/server-heartbeat-message.bin
BOE_TEST_DATA += test/protocol/boe/replay-complete-message.bin

%.bin: %.hexdump
	$(E) "  XXD     " $@
	$(Q) $(XXD) -p -r $< > $@

BOE_TEST_DATA:

$(TEST_RUNNER_C): $(FORCE)
	$(E) "  GEN     " $@
	$(Q) $(shell scripts/gen-test-runner "$(TEST_SRC)" > $@)

$(TEST_SUITE_H): $(FORCE)
	$(E) "  GEN     " $@
	$(Q) $(shell scripts/gen-test-proto "$(TEST_SRC)" > $@)

$(TEST_RUNNER_OBJ): $(TEST_RUNNER_C)

$(TEST_PROGRAM): $(TEST_SUITE_H) $(TEST_DEPS) $(TEST_RUNNER_OBJ) $(TEST_OBJS) $(LIB_FILE) $(BOE_TEST_DATA)
	$(E) "  LINK    " $@
	$(E) "  LINK    " $<
	$(Q) $(CC) $(TEST_OBJS) $(TEST_RUNNER_OBJ) $(TEST_LIBS) -o $(TEST_PROGRAM)

check: $(TEST_PROGRAM) $(PROGRAMS)
	$(E) "  CHECK"
	$(Q) ./$(TEST_PROGRAM)
	$(Q) $(PYTHON) tools/test.py
.PHONY: check

clean:
	$(E) "  CLEAN"
	$(Q) find . -name "*.o" | xargs rm -f
	$(Q) rm -f $(LIB_FILE) $(LIB_OBJS) $(LIB_DEPS)
	$(Q) rm -f $(PROGRAMS) $(OBJS) $(DEPS) $(TEST_PROGRAM) $(TEST_SUITE_H) $(TEST_OBJS) $(TEST_DEPS) $(TEST_RUNNER_C) $(TEST_RUNNER_OBJ)
	$(Q) rm -f $(BOE_TEST_DATA)
.PHONY: clean

tags: FORCE
	$(E) "  TAGS"
	$(Q) rm -f tags
	$(Q) ctags-exuberant -a *.c
	$(Q) ctags-exuberant -a -R include

PHONY += FORCE

FORCE:

# Deps
-include $(DEPS)

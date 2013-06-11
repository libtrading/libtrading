# The default target of this Makefile:
all:
#
# Define WERROR=0 to disable -Werror.

LIBTRADING-VERSION-FILE: .FORCE-LIBTRADING-VERSION-FILE
	sh tools/gen-version-file

uname_S	:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_R	:= $(shell sh -c 'uname -r 2>/dev/null || echo not')

DESTDIR=
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include

# External programs
CC	?= gcc
LD	:= $(CC)
AR	:= ar
XXD	?= xxd

ifneq ($(WERROR),0)
	CFLAGS_WERROR = -Werror
endif

PREFIX ?= $(HOME)

# Set up source directory for GNU Make
libdir		:= lib
prtdir		:= lib/proto

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
CFLAGS		:= -I$(CURDIR)/include -Wall $(EXTRA_WARNINGS) $(CFLAGS_WERROR) -g -O3 -std=gnu99

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
PROGRAMS += tools/fast/fast_client
PROGRAMS += tools/fast/fast_orderbook
PROGRAMS += tools/fast/fast_parser
PROGRAMS += tools/fast/fast_server
PROGRAMS += tools/fix/fix_client
PROGRAMS += tools/fix/fix_server
PROGRAMS += tools/test-fix-client
PROGRAMS += tools/test-fix-server
PROGRAMS += tools/tape/tape

DEFINES =
INCLUDES = $(shell sh -c 'xml2-config --cflags')

EXTRA_LIBS += $(shell sh -c 'xml2-config --libs')

EXTRA_LIBS += -lz

EXTRA_LIBS += -lncurses

ifeq ($(uname_S),Linux)
	DEFINES += -D_GNU_SOURCE

	EXTRA_LIBS += -lrt
endif

ifeq ($(uname_S),Darwin)
	CONFIG_OPTS += -DCONFIG_NEED_CLOCK_GETTIME=1
	COMPAT_OBJS += lib/compat/clock_gettime.o
endif

test-fix-server_EXTRA_DEPS += lib/die.o

test-fix-client_EXTRA_DEPS += lib/die.o

fix_client_EXTRA_DEPS += lib/die.o
fix_client_EXTRA_DEPS += tools/fix/test.o

fix_server_EXTRA_DEPS += lib/die.o
fix_server_EXTRA_DEPS += tools/fix/test.o

fast_client_EXTRA_DEPS += lib/die.o
fast_client_EXTRA_DEPS += tools/fast/test.o

fast_server_EXTRA_DEPS += lib/die.o
fast_server_EXTRA_DEPS += tools/fast/test.o

fast_parser_EXTRA_DEPS += lib/die.o
fast_parser_EXTRA_DEPS += tools/fast/test.o

tape_EXTRA_DEPS += tools/tape/builtin-check.o

CFLAGS += $(DEFINES)
CFLAGS += $(INCLUDES)
CFLAGS += $(CONFIG_OPTS)

DEPS		:= $(patsubst %.o,%.d,$(OBJS))

LIB_FILE := libtrading.a

LIBS := $(LIB_FILE)

LIB_H += array.h
LIB_H += buffer.h
LIB_H += compat.h
LIB_H += order_book.h
LIB_H += proto/bats_pitch_message.h
LIB_H += proto/boe_message.h
LIB_H += proto/fast_book.h
LIB_H += proto/fast_feed.h
LIB_H += proto/fast_message.h
LIB_H += proto/fast_session.h
LIB_H += proto/fix_message.h
LIB_H += proto/fix_message.h
LIB_H += proto/lse_itch_message.h
LIB_H += proto/mbt_quote_message.h
LIB_H += proto/nasdaq_itch40_message.h
LIB_H += proto/nasdaq_itch41_message.h
LIB_H += proto/nyse_taq_message.h
LIB_H += proto/omx_itch186_message.h
LIB_H += proto/ouch42_message.h
LIB_H += proto/soupbin3_session.h
LIB_H += proto/xdp_message.h
LIB_H += read-write.h
LIB_H += types.h

LIB_OBJS	+= lib/buffer.o
LIB_OBJS	+= lib/order_book.o
LIB_OBJS	+= lib/mmap-buffer.o
LIB_OBJS	+= lib/read-write.o
LIB_OBJS	+= lib/proto/bats_pitch_message.o
LIB_OBJS	+= lib/proto/boe_message.o
LIB_OBJS	+= lib/proto/fix_message.o
LIB_OBJS	+= lib/proto/fix_session.o
LIB_OBJS	+= lib/proto/fast_book.o
LIB_OBJS	+= lib/proto/fast_feed.o
LIB_OBJS	+= lib/proto/fast_message.o
LIB_OBJS	+= lib/proto/fast_session.o
LIB_OBJS	+= lib/proto/fast_template.o
LIB_OBJS	+= lib/proto/mbt_quote_message.o
LIB_OBJS	+= lib/proto/nasdaq_itch40_message.o
LIB_OBJS	+= lib/proto/nasdaq_itch41_message.o
LIB_OBJS	+= lib/proto/nyse_taq_message.o
LIB_OBJS	+= lib/proto/omx_itch186_message.o
LIB_OBJS	+= lib/proto/ouch42_message.o
LIB_OBJS	+= lib/proto/soupbin3_session.o
LIB_OBJS	+= lib/proto/xdp_message.o
LIB_OBJS	+= lib/proto/lse_itch_message.o

LIB_OBJS	+= $(COMPAT_OBJS)

LIB_DEPS	:= $(patsubst %.o,%.d,$(LIB_OBJS))

TEST_PROGRAM	:= test-trade
TEST_SUITE_H	:= tools/test/test-suite.h
TEST_RUNNER_C	:= tools/test/test-runner.c
TEST_RUNNER_OBJ := tools/test/test-runner.o

TEST_OBJS += tools/test/boe-test.o
TEST_OBJS += tools/test/harness.o
TEST_OBJS += tools/test/mbt_quote_message-test.o
TEST_OBJS += tools/test/unparse-test.o

TEST_SRC	:= $(patsubst %.o,%.c,$(TEST_OBJS))
TEST_DEPS	:= $(patsubst %.o,%.d,$(TEST_OBJS))

TEST_LIBS := $(LIBS)

INST_PROGRAMS += libtrading-config

define INSTALL_EXEC
	install -v $1 $(DESTDIR)$2/$1 || exit 1;
endef

define INSTALL_FILE
	install -v -m 644 $1 $(DESTDIR)$2/$1 || exit 1;
endef

define INSTALL_HEADER
	install -v -m 644 include/libtrading/$1 $(DESTDIR)$2/$1 || exit 1;
endef

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

libtrading-config.o: libtrading-config.c LIBTRADING-VERSION-FILE
	$(E) "  CC      " $@
	$(Q) $(CC) -DPREFIX=\"$(PREFIX)\" -include LIBTRADING-VERSION-FILE -c $(CFLAGS) $< -o $@
.PHONY: libtrading-config.o

libtrading-config: libtrading-config.o
	$(E) "  LINK    " $@
	$(Q) $(LD) $(LDFLAGS) -o $@ $^ $($(notdir $@)_EXTRA_LIBS) $(EXTRA_LIBS)

install: all libtrading-config
	$(E) "  INSTALL "
	$(Q) install -d $(DESTDIR)$(BINDIR)
	$(Q) install -d $(DESTDIR)$(LIBDIR)
	$(Q) install -d $(DESTDIR)$(INCLUDEDIR)/libtrading/proto
	$(Q) $(foreach f,$(INST_PROGRAMS),$(call INSTALL_EXEC,$f,$(BINDIR)))
	$(Q) $(foreach f,$(LIBS),$(call INSTALL_FILE,$f,$(LIBDIR)))
	$(Q) $(foreach f,$(LIB_H),$(call INSTALL_HEADER,$f,$(INCLUDEDIR)/libtrading))
.PHONY: install

%.d: %.c
	$(Q) $(CC) -M -MT $(patsubst %.d,%.o,$@) $(CFLAGS) $< -o $@

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@


$(foreach p,$(PROGRAMS),$(eval $(p): $($(notdir $p)_EXTRA_DEPS) $(LIBS)))
$(PROGRAMS): % : %.o
	$(E) "  LINK    " $@
	$(Q) $(LD) $(LDFLAGS) -o $@ $^ $($(notdir $@)_EXTRA_LIBS) $(EXTRA_LIBS)

$(LIB_FILE): $(LIB_DEPS) $(LIB_OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(LIB_OBJS)

test: $(TEST_PROGRAM)
	$(E) "  TEST"
	$(Q) ./$(TEST_PROGRAM)
.PHONY: test

BOE_TEST_DATA += tools/test/protocol/boe/login-request-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/logout-request-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/client-heartbeat-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/login-response-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/logout-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/server-heartbeat-message.bin
BOE_TEST_DATA += tools/test/protocol/boe/replay-complete-message.bin

%.bin: %.hexdump
	$(E) "  XXD     " $@
	$(Q) $(XXD) -p -r $< > $@

BOE_TEST_DATA:

$(TEST_RUNNER_C): $(FORCE)
	$(E) "  GEN     " $@
	$(Q) $(shell tools/gen-test-runner "$(TEST_SRC)" > $@)

$(TEST_SUITE_H): $(FORCE)
	$(E) "  GEN     " $@
	$(Q) $(shell tools/gen-test-proto "$(TEST_SRC)" > $@)

$(TEST_RUNNER_OBJ): $(TEST_RUNNER_C)

$(TEST_PROGRAM): $(TEST_SUITE_H) $(TEST_DEPS) $(TEST_RUNNER_OBJ) $(TEST_OBJS) $(LIB_FILE) $(BOE_TEST_DATA)
	$(E) "  LINK    " $@
	$(E) "  LINK    " $<
	$(Q) $(CC) $(TEST_OBJS) $(TEST_RUNNER_OBJ) $(TEST_LIBS) $(EXTRA_LIBS) -o $(TEST_PROGRAM)

check: $(TEST_PROGRAM) $(PROGRAMS)
	$(E) "  CHECK"
	$(Q) ./$(TEST_PROGRAM)
	$(Q) $(PYTHON) tools/test.py
.PHONY: check

clean:
	$(E) "  CLEAN"
	$(Q) find . -name "*.o" | xargs rm -f
	$(Q) rm -f $(LIB_FILE) $(LIB_OBJS) $(LIB_DEPS)
	$(Q) rm -f $(PROGRAMS) $(INST_PROGRAMS) $(OBJS) $(DEPS) $(TEST_PROGRAM) $(TEST_SUITE_H) $(TEST_OBJS) $(TEST_DEPS) $(TEST_RUNNER_C) $(TEST_RUNNER_OBJ)
	$(Q) rm -f $(BOE_TEST_DATA)
.PHONY: clean

tags: FORCE
	$(E) "  TAGS"
	$(Q) rm -f tags
	$(Q) ctags-exuberant -a $(join lib lib/proto, /*.c /*.c)
	$(Q) ctags-exuberant -a -R include

PHONY += FORCE

FORCE:

.PHONY: .FORCE-LIBTRADING-VERSION-FILE

# Deps
-include $(DEPS)

# External programs:
CC		?= gcc
AR		?= ar

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

LIBRARY		:= libfix.a

EXTRA_WARNINGS += -Wbad-function-cast
EXTRA_WARNINGS += -Wdeclaration-after-statement
EXTRA_WARNINGS += -Wformat
EXTRA_WARNINGS += -Wformat-security
EXTRA_WARNINGS += -Wformat-y2k
EXTRA_WARNINGS += -Winit-self
EXTRA_WARNINGS += -Wmissing-declarations
EXTRA_WARNINGS += -Wmissing-prototypes
EXTRA_WARNINGS += -Wnested-externs
EXTRA_WARNINGS += -Wno-system-headers
EXTRA_WARNINGS += -Wold-style-definition
EXTRA_WARNINGS += -Wpacked
EXTRA_WARNINGS += -Wredundant-decls
EXTRA_WARNINGS += -Wshadow
EXTRA_WARNINGS += -Wstrict-aliasing=3
EXTRA_WARNINGS += -Wstrict-prototypes
EXTRA_WARNINGS += -Wswitch-default
EXTRA_WARNINGS += -Wswitch-enum
EXTRA_WARNINGS += -Wundef
EXTRA_WARNINGS += -Wwrite-strings
EXTRA_WARNINGS += -Wcast-align

OBJS		+= buffer.o
OBJS		+= field.o
OBJS		+= message.o
OBJS		+= parser.o
OBJS		+= session.o

CFLAGS		+= -Iinclude -Wall -g -O3
CFLAGS		+= $(EXTRA_WARNINGS)

all: $(LIBRARY) examples
.PHONY: all

$(LIBRARY): $(OBJS)
	$(E) "  AR      " $@
	$(Q) rm -f $@ && $(AR) rcs $@ $(OBJS)

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c $< -o $@

examples: $(LIBRARY)
	$(E) "  MAKE    " $@
	$(Q) $(MAKE) --no-print-directory -C $@
.PHONY: examples

check: all
	$(PYTHON) tools/test.py
.PHONY: check

clean:
	$(E) "  CLEAN   "
	$(Q) rm -f $(OBJS)
	$(Q) rm -f $(LIBRARY)
	$(Q) $(MAKE) --no-print-directory -s -C examples clean
.PHONY: clean

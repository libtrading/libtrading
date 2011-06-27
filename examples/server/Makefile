CC		?= gcc
LINK		?= gcc

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

INCLUDES	+= -I../../include

LIBS		:= -L../../ -lfix

CFLAGS		+= $(EXTRA_WARNINGS)
CFLAGS		+= $(INCLUDES)

PROGRAM		:= fix-server

OBJS		+= main.o

all: $(PROGRAM)
.PHONY: all

$(PROGRAM): $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(LINK) $(OBJS) $(LIBS) -o $@

$(OBJS):

%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f $(PROGRAM)
.PHONY: clean

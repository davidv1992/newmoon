include make.config

#Setup convenience parameters
LIB_DEPENDENCIES:=libs/libfree.a
MOD_DEPENDENCIES:=module_common/start.o

#Setup gather variables
ALL_TARGETS:=
ALL_OBJS:=module_common/start.o
CLEAN_FILES:=
INCLUDES:=

include libfree/make.config

include boot/make.config
include modcore/make.config

#Produce final flags and such
CFLAGS:=$(CFLAGS) -ffreestanding -mgeneral-regs-only -nostdinc -Wall -Wextra -Werror -O3 -I$(CC_BASE_INCLUDE)
CPPFLAGS:=$(CPPFLAGS)
LDFLAGS:=$(LDFLAGS)
LDFLAGS_MODULE:= module_common/start.o $(LDFLAGS) -r -d -e _module_entry
LIBS:=$(LIBS) -L$(LD_BASE_LIB) -L./libs -lgcc -lfree

# General production rules
%.o : %.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(INCLUDES) $(CFLAGS) $(CPPFLAGS)

%.o : %.S
	$(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS) $(CPPFLAGS)

# General targets
.PHONY: clean all
.DEFAULT_GOAL=all

all: $(ALL_TARGETS)

clean:
	rm -f $(ALL_OBJS)
	rm -f $(ALL_OBJS:.o=.d)
	rm -f $(CLEAN_FILES)

#include dependency files
-include $(ALL_OBJS:.o=.d)
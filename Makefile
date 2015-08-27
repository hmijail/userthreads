WARNINGS := -Wall #-Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes

CFLAGS ?= -std=gnu99 -Wall -g $(WARNINGS)

OBJDIR := obj
SRCDIR := src

# Eclipse needs to see the full command lines
ifeq ($(VERBOSE),1)
    SILENCER := 
else
    SILENCER := @
endif

_SRCS := uthreads.c main.c 
SRCS := $(patsubst %,$(SRCDIR)/%,$(_SRCS))
OBJS := $(patsubst %,$(OBJDIR)/%,$(_SRCS:c=o))

# generate phony deps during compilation
CFLAGS += -MMD -MP	
DEPS := $(patsubst %,$(OBJDIR)/%,$(_SRCS:c=d))

all: main

createdir:
	$(SILENCER)mkdir -p $(OBJDIR)

main: $(OBJS) 
	@echo " LINK $^"
	$(SILENCER)$(CC) $(CFLAGS) -o $@ $^ 

$(OBJDIR)/%.o: $(SRCDIR)/%.c | createdir
	@echo " CC $<"
	$(SILENCER)$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	$(RM) -f *~ core main
	$(RM) -r $(OBJDIR)

.PHONY: clean all

-include $(DEPS)
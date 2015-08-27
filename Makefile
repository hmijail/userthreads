CC ?= gcc
WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
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

CFLAGS += -MMD -MP	#generate phony deps during compilation
DEPS := $(patsubst %,$(OBJDIR)/%,$(_SRCS:c=d))

all: main

createdir:
	$(SILENCER)mkdir -p $(OBJDIR)

#main: $(SRCDIR)/main.c

main: $(OBJS) 
	@echo " LINK $^"
	$(SILENCER)$(CC) $(CFLAGS) -o $@ $^ 

#build the src/*.c files into obj/, and create the dir first
$(OBJDIR)/%.o: $(SRCDIR)/%.c | createdir
	@echo " CC $<"
	$(SILENCER)$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	$(RM) -f *~ core main
	$(RM) -r $(OBJDIR)

.PHONY: clean all

-include $(DEPS)
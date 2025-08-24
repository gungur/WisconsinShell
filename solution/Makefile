# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=gnu18
CFLAGS-TARG = $(CFLAGS) -O2
CFLAGS-DBG = $(CFLAGS) -Og -ggdb

TARG = wsh
SRCS = $(TARG).c $(TARG).h

LOGIN = gungurthi
SUBMITPATH = ~cs537-1/handin/$(LOGIN)/p3

# Targets
.PHONY: all
all: $(TARG) $(TARG)-dbg

$(TARG): $(SRCS)
	$(CC) $(CFLAGS-TARG) $< -o $@

$(TARG)-dbg: $(SRCS)
	$(CC) $(CFLAGS-DBG) $< -o $@

.PHONY: clean
clean:
	rm -f $(TARG) $(TARG)-dbg

.PHONY: submit
submit:
	cp -r ~/private/cs537/p3 $(SUBMITPATH)
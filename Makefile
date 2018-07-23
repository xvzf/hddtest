.PHONY: all clean

CC=gcc
DEBUG=-O3
CFLAGS=-Werror -Wall -pedantic -std=gnu11 $(DEBUG) $(OPTIONAL_CFLAGS)
# Use for development:
# LDFLAGS=-fsanitize=address -lm
# CFLAGS=-Werror -Wall -pedantic -fsanitize=address -std=gnu11 $(DEBUG) $(OPTIONAL_CFLAGS)

OPTIONAL_CFLAGS= -Wcomment -Wformat -Wimplicit -Wparentheses -Wreturn-type -Wunused -Wstrict-prototypes -Wmissing-prototypes

all: objects

objects: hddtest

hddtest: hddtest.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

dist-clean:
	rm -f $(shell find . -not -name "." -not -name "*.[ch]" -not -name "*.py" -not -name "Makefile")

clean:
	rm -f ./*.o

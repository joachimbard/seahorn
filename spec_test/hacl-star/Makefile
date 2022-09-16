include ../Makefile.common

INCFLAGS := -I. -Ikaramel/include -Ikaramel/krmllib/dist/minimal/

CFILES := $(sort $(wildcard *test.c))
LLFILES := $(CFILES:.c=.ll)

%.ll: %.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: clean
clean:
	rm -f $(LLFILES)

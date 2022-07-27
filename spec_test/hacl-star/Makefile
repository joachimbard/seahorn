include ../Makefile

INCFLAGS := -I. -Ikaramel/include -Ikaramel/krmllib/dist/minimal/

CFILES := $(sort $(wildcard *test.c))
LLFILES := $(CFILES:.c=.ll)

.default: compile

%.ll: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)
	

.PHONY: clean-tests
clean-tests:
	rm -f $(LLFILES)

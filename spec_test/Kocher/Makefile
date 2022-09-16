include ../Makefile.common

CFILES:=$(sort $(wildcard *.c))
LLFILES:=$(CFILES:.c=.ll)

%.ll: %.c
	$(CC) -I../../include $(CFLAGS) $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)

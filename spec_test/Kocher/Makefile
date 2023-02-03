include ../Makefile.common

CFILES:=$(sort $(wildcard *.c))
LLFILES:=$(CFILES:.c=.ll)

%.ll: %.c
	$(CC) -I../../include $(CFLAGS) -S -emit-llvm $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-exec
build-exec: ;

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)

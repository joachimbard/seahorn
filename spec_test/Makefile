export SHELL=/bin/bash

export CC:=clang-10
export CFLAGS:=-Wall -S -emit-llvm
export OPTLVL:=-O1

DIRS:=Kocher openssl manual
CFILES:=$(foreach d, $(DIRS), $(sort $(wildcard $d/*.c)))
LLFILES:=$(CFILES:.c=.ll)

SEAHORN:=../build/run/bin/seahorn
SCRIPT:=run-test.py
TMPDIR:=tmp

.default: compile

%.ll: %.c
	$(CC) $(CFLAGS) -I../include $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)
	$(MAKE) $@ -C hacl-star

.PHONY: test
test: compile $(SCRIPT)
	python3 $(SCRIPT) --all

.PHONY: clean-tests clean-tmpdir clean-all
clean-tests:
	rm -f $(LLFILES)
	$(MAKE) $@ -C hacl-star

clean-tmpdir:
	rm -f $(TMPDIR)/*.ll
	rm -f $(TMPDIR)/*.smt2
	rm -f $(TMPDIR)/*.out
	rm -f $(TMPDIR)/*.err

clean-all: clean-tests clean-tmpdir

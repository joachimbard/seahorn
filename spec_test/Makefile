export SHELL=/bin/bash

export CC:=clang-10
export CFLAGS:=-Wall -S -emit-llvm
export OPTLVL:=-O2

DIRS:=Kocher openssl manual
AUXFILES:=$(foreach d, $(DIRS), $(sort $(wildcard $d/*aux.c)))
CFILES:=$(foreach d, $(DIRS), $(sort $(filter-out $(AUXFILES), $(wildcard $d/*.c))))
LLFILES:=$(CFILES:.c=.ll)
TESTFILES:=$(LLFILES:.ll=)

SEAHORN:=../build/run/bin/seahorn
SCRIPT:=run-test.py
TMPDIR:=tmp

.default: compile

%.ll: %.c
	$(CC) $(CFLAGS) -I../include $(OPTLVL) $< -o $@

%.o: %.c
	$(CC) -Wall -c $(OPTLVL) $< -o $@

%: %.ll $(AUXFILES:.c=.o)
	$(CC) -Wall -O0 $^ -o $@

.PHONY: compile
compile: $(LLFILES)
	$(MAKE) $@ -C hacl-star

.PHONY: compile-aux
compile-aux: $(TESTFILES)

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

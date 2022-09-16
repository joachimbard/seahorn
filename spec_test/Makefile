include Makefile.common

# DIRS:=Kocher openssl hacl-star
DIRS:=Kocher openssl
SEAHORN:=../build/run/bin/seahorn
SCRIPT:=run-test.py
TMPDIR:=tmp

.PHONY: compile
compile:
	@for d in $(DIRS); do $(MAKE) $@ -C $$d; done

.PHONY: build-exec
build-exec:
	@for d in $(DIRS); do $(MAKE) $@ -C $$d; done

.PHONY: test
test: compile $(SCRIPT)
	python3 $(SCRIPT) --all

.PHONY: clean clean-tmpdir clean-all
clean:
	@for d in $(DIRS); do $(MAKE) $@ -C $$d; done

clean-tmpdir:
	rm -f $(TMPDIR)/*.ll
	rm -f $(TMPDIR)/*.smt2
	rm -f $(TMPDIR)/*.out
	rm -f $(TMPDIR)/*.err

clean-all: clean-tmpdir clean

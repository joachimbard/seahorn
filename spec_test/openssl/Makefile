include ../Makefile.common

AUXFILES:=$(sort $(wildcard *aux.c))
CFILES:=$(sort $(filter-out $(AUXFILES), $(wildcard *.c)))
LLFILES:=$(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)
AES_EXECS:=$(filter openssl-aes%, $(EXECS))
BN_EXECS:=$(filter openssl-bn%, $(EXECS))

%.ll: %.c
	$(CC) $(CFLAGS) $(OPTLVL) $< -o $@

%.o: %.c
	$(CC) -Wall -c $(OPTLVL) $< -o $@

$(AES_EXECS): %: %.ll openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

$(BN_EXECS): %: %.ll openssl-bn-aux.c
	$(CC) -Wall -O0 $^ -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-aes-exec
build-aes-exec: $(AES_EXECS)

.PHONY: build-bn-exec
build-bn-exec: $(BN_EXECS)

.PHONY: build-exec
build-exec: $(EXECS)

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f $(EXECS)

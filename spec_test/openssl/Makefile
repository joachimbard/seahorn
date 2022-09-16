include ../Makefile.common

AUXFILES:=$(sort $(wildcard *aux.c))
CFILES:=$(sort $(filter-out $(AUXFILES), $(wildcard *.c)))
LLFILES:=$(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)

%.ll: %.c
	$(CC) $(CFLAGS) $(OPTLVL) $< -o $@

%.o: %.c
	$(CC) -Wall -c $(OPTLVL) $< -o $@

openssl-aes_encrypt: openssl-aes_encrypt.ll openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

openssl-aes_encrypt_ct: openssl-aes_encrypt_ct.ll openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

openssl-bn_mul: openssl-bn_mul.ll openssl-bn-aux.c
	$(CC) -Wall -O0 $^ -o $@

# $(EXECS): %: %.ll
# 	@echo no rule

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-exec
build-exec: $(EXECS)

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f $(EXECS)

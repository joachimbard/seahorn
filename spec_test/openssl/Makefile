include ../Makefile.common

AUXFILES:=$(sort $(wildcard *aux.c))
CFILES:=$(sort $(filter-out $(AUXFILES), $(wildcard *.c)))
LLFILES:=$(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)

%.ll: %.c
	$(CC) $(CFLAGS) $(OPTLVL) $< -o $@

%.o: %.c
	$(CC) -Wall -c $(OPTLVL) $< -o $@

openssl/openssl-aes_encrypt: openssl/openssl-aes_encrypt.ll openssl/openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

openssl/openssl-aes_encrypt_ct: openssl/openssl-aes_encrypt_ct.ll openssl/openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

openssl/openssl-bn_mul: openssl/openssl-bn_mul.ll openssl/openssl-bn-aux.c
	$(CC) -Wall -O0 $^ -o $@

# $(EXECS): %: %.ll
# 	@echo no rule

.PHONY: compile
compile: $(LLFILES)

.PHONY: compile-aux
compile-aux: $(EXECS)

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f $(EXECS)

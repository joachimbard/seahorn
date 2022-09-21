include ../Makefile.common

AUXFILES:=$(sort $(wildcard *aux.c))
CFILES:=$(sort $(filter-out $(AUXFILES), $(wildcard *.c)))
LLFILES:=$(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)
AES_EXECS:=$(filter openssl-aes%, $(EXECS))
BN_EXECS:=$(filter openssl-bn%, $(EXECS))
FIXED_LLFILES:=$(wildcard fixed/*.ll)
FIXED_EXECS:=$(FIXED_LLFILES:.ll=)

%.ll: %.c
	$(CC) $(CFLAGS) $(OPTLVL) $< -o $@

#%.o: %.c
#	$(CC) -Wall -c $(OPTLVL) $< -o $@

$(AES_EXECS): %: %.ll openssl-aes-aux.c
	$(CC) -Wall -O0 $^ -o $@

$(BN_EXECS): %: %.ll openssl-bn-aux.c
	$(CC) -Wall -O0 $^ -o $@

fixed/openssl-aes_encrypt_before-memory_opt_fixed: fixed/openssl-aes_encrypt_before-memory_opt_fixed.ll openssl-aes-aux.c
	$(CC) -Wall -O0 -DNO_SET_KEY -DOPENSSL_NO_AES_CONST_TIME $^ -o $@

fixed/openssl-aes_encrypt_ct_before-memory_opt_fixed: fixed/openssl-aes_encrypt_ct_before-memory_opt_fixed.ll openssl-aes-aux.c
	$(CC) -Wall -O0 -DNO_SET_KEY $^ -o $@

fixed/openssl-aes_cbc_encrypt_before-memory_opt_fixed: fixed/openssl-aes_cbc_encrypt_before-memory_opt_fixed.ll openssl-aes-aux.c
	$(CC) -Wall -O0 -DNO_SET_KEY -DOPENSSL_NO_AES_CONST_TIME $^ -o $@

fixed/openssl-aes_cbc_encrypt_ct_before-memory_opt_fixed: fixed/openssl-aes_cbc_encrypt_ct_before-memory_opt_fixed.ll openssl-aes-aux.c
	$(CC) -Wall -O0 -DNO_SET_KEY $^ -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-aes-exec
build-aes-exec: $(AES_EXECS)

.PHONY: build-bn-exec
build-bn-exec: $(BN_EXECS)

.PHONY: build-fixed-exec
build-fixed-exec: $(FIXED_EXECS)

.PHONY: build-exec
build-exec: $(EXECS)

.PHONY: test
test: compile

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f $(EXECS)

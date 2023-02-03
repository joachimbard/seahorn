include ../Makefile.common

AUXFILES:=$(sort $(wildcard *aux.c))
#CFILES:=$(sort $(filter-out $(AUXFILES), $(wildcard *.c)))
CFILES:=openssl-aes_encrypt_non-ct.c openssl-aes_encrypt_ct.c \
				openssl-aes_cbc_encrypt_non-ct.c openssl-aes_cbc_encrypt_ct.c \
				openssl-bn_mul_part.c openssl-bn_sqr_part.c openssl-bn_exp_part.c

#				openssl-bn_exp.c openssl-bn_exp_old.c openssl-bn_mul.c \
				openssl-bn_mul_no-sec_no-err.c openssl-bn_mul_no-ctx_no-err.c \
				openssl-bn_sqr.c openssl-bn_mod_exp.c openssl-rsa_ossl_private_encrypt.c
LLFILES:=$(CFILES:.c=.ll)
OBJFILES:=$(CFILES:.c=.o)
EXECS:=$(LLFILES:.ll=)
AES_EXECS:=$(filter openssl-aes%, $(EXECS))
BN_EXECS:=$(filter openssl-bn%, $(EXECS))
FIXED_LLFILES:=$(wildcard fixed/*.ll)
FIXED_EXECS:=$(FIXED_LLFILES:.ll=)

%.ll: %.c
	$(CC) $(CFLAGS) $(OPTLVL) -S -emit-llvm $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(OPTLVL) -c $< -o $@

$(AES_EXECS): %: %.ll openssl-aes-aux.o
#	$(CC) $(CFLAGS) -O0 $^ -o $@
	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

$(BN_EXECS): %: %.o openssl-bn-aux.o
	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

#fixed/openssl-aes_encrypt_non-ct_%: fixed/openssl-aes_encrypt_non-ct_%.ll openssl-aes-aux.o
#	$(CC) $(CFLAGS) -O0 -DNO_SET_KEY -DOPENSSL_NO_AES_CONST_TIME $^ -o $@
#
#fixed/openssl-aes_encrypt_ct_%: fixed/openssl-aes_encrypt_ct_%.ll openssl-aes-aux.o
#	$(CC) $(CFLAGS) -O0 -DNO_SET_KEY $^ -o $@

fixed/openssl-aes_cbc_encrypt_non-ct_%: fixed/openssl-aes_cbc_encrypt_non-ct_%.ll openssl-aes-aux.o openssl-aes_non-ct_key.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

fixed/openssl-aes_cbc_encrypt_ct_%: fixed/openssl-aes_cbc_encrypt_ct_%.ll openssl-aes-aux.o openssl-aes_ct_key.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@
#	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

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
test:
	@echo "Not implemented"
	@echo $(CFILES)

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f *.o
	rm -f $(EXECS)
	rm -f $(FIXED_EXECS)

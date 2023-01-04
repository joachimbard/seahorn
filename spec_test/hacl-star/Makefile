include ../Makefile.common

INCFLAGS := -I. -Ikaramel/include -Ikaramel/krmllib/dist/minimal/

CFILES := Hacl_Curve25519_64_ecdh_test.c Hacl_Hash_SHA2_hash_224_test.c Hacl_Hash_SHA2_hash_256_test.c
LLFILES := $(CFILES:.c=.ll)

%.ll: %.c
	$(CC) $(INCFLAGS) $(CFLAGS) -S -emit-llvm $(OPTLVL) $< -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: clean
clean:
	rm -f $(LLFILES)

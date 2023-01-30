include ../Makefile.common

INCFLAGS := -I. -Ikaramel/include -Ikaramel/krmllib/dist/minimal/

CFILES := Hacl_Ed25519_sign.c Hacl_Curve25519_51_ecdh.c Hacl_Curve25519_64_ecdh.c Hacl_Curve25519_64_Slow_ecdh.c Hacl_Hash_SHA2_hash_224.c Hacl_Hash_SHA2_hash_256.c Hacl_Chacha20_encrypt.c Hacl_Poly1305_32_mac.c Hacl_Poly1305_256_mac.c
LLFILES := $(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)

Hacl_Poly1305_256_mac.ll: Hacl_Poly1305_256_mac.c
	$(CC) $(INCFLAGS) $(CFLAGS) -mavx2 $(OPTLVL) -S -emit-llvm $< -o $@

%.ll: %.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(OPTLVL) -S -emit-llvm $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(OPTLVL) -c $< -o $@

Hacl_Chacha20_encrypt: Hacl_Chacha20_encrypt.ll hacl-chacha-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

fixed/Hacl_Chacha20_encrypt_%: fixed/Hacl_Chacha20_encrypt_%.ll hacl-chacha-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

Hacl_Poly1305_%: Hacl_Poly1305_%.ll hacl-poly-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

fixed/Hacl_Poly1305_%: fixed/Hacl_Poly1305_%.ll hacl-poly-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

Hacl_Curve25519_%: Hacl_Curve25519_%.ll hacl-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

fixed/Hacl_Curve25519_%: fixed/Hacl_Curve25519_%.ll hacl-aux.o
	$(CC) $(CFLAGS) $^ -o $@
#	$(CC) $(CFLAGS) -O0 $^ -o $@

#$(EXECS): %: %.ll hacl-aux.o
#	$(CC) $(CFLAGS) -O0 $^ -o $@
#	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-exec
build-exec: $(EXECS)

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f *.o
	rm -f $(EXECS)

include ../Makefile.common

INCFLAGS := -I. -Ikaramel/include -Ikaramel/krmllib/dist/minimal/

CFILES := Hacl_Curve25519_64_ecdh_test.c Hacl_Curve25519_64_Slow_ecdh_test.c Hacl_Hash_SHA2_hash_224_test.c Hacl_Hash_SHA2_hash_256_test.c
LLFILES := $(CFILES:.c=.ll)
EXECS:=$(LLFILES:.ll=)

%.ll: %.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(OPTLVL) -S -emit-llvm $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(OPTLVL) -c $< -o $@

$(EXECS): %: %.ll hacl-aux.o
#	$(CC) $(CFLAGS) -O0 $^ -o $@
	$(CC) $(CFLAGS) $(OPTLVL) $^ -o $@

.PHONY: compile
compile: $(LLFILES)

.PHONY: build-exec
build-exec: $(EXECS)

.PHONY: clean
clean:
	rm -f $(LLFILES)
	rm -f *.o
	rm -f $(EXECS)

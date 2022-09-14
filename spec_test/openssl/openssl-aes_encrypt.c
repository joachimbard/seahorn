#include "openssl-aes.h"
#include "openssl-aes-aux.h"
#include <seahorn/seahorn.h>

/* This controls loop-unrolling in aes_core.c */
// #define FULL_UNROLL
#define OPENSSL_NO_AES_CONST_TIME

#include "openssl-aes_core_impl.h"

int main(int argc, char **argv) {
  init_seed(argv);
  unsigned char *in = init_array();
  unsigned char *out = init_array();
  AES_KEY *key = init_key();

  AES_encrypt(in, out, key);

  // to avoid optimizations
  display_aes(out);
  return 0;
}

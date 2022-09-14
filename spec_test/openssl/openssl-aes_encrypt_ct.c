#include <stdint.h>
#include <string.h>
#include "openssl-aes.h"
#include "openssl-aes-aux.h"
#include <seahorn/seahorn.h>

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

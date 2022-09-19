#include "openssl-aes.h"
#include "openssl-aes-aux.h"

#include "openssl-aes_core_impl.h"

int main(int argc, char **argv) {
  init_seed(argv);
  unsigned char *in = init_array(true);
  unsigned char *out = init_array(false);
  AES_KEY key;
  init_key(&key);

  start_clock();
  AES_encrypt(in, out, &key);
  end_clock();

  // to avoid optimizations
  display_aes(out);
  return 0;
}

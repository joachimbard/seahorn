#include "openssl-aes.h"
#include "openssl-aes-aux.h"

#include "openssl-aes_core_impl.h"
#include "openssl-aes_cbc128_impl.h"
#include "openssl-aes_cbc_encrypt_impl.h"

int main(int argc, char **argv) {
  init_seed(argv);
  unsigned char *in = init_array(true);
  unsigned char *out = init_array(false);
  unsigned char *ivec = init_ivec();
  size_t len = get_length();
  AES_KEY key;
  init_key(&key);

  start_clock();
  AES_cbc_encrypt(in, out, len, &key, ivec, true);
  end_clock();

  // to avoid optimizations
  display_aes(out);
  return 0;
}

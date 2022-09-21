/* This controls loop-unrolling in aes_core.c */
// #define FULL_UNROLL
#define OPENSSL_NO_AES_CONST_TIME

#include "openssl-aes.h"
#include "openssl-aes-aux.h"

#include "openssl-aes_core_impl.h"
#include "openssl-aes_cbc128_impl.h"

// from crypto/aes/aes_cbc.c
void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
                     size_t len, const AES_KEY *key,
                     unsigned char *ivec, const int enc)
{

    if (enc)
        CRYPTO_cbc128_encrypt(in, out, len, key, ivec,
                              (block128_f) AES_encrypt);
    else
        CRYPTO_cbc128_decrypt(in, out, len, key, ivec,
                              (block128_f) AES_decrypt);
}

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

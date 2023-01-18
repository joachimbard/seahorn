#include "openssl-rsa.h"

#include "openssl-bn_impl.h"
#include "openssl-bn_mul_impl.h"
#include "openssl-bn_sqr_impl.h"
#include "openssl-bn_mont_impl.h"

#include "openssl-rsa_ossl_private_encrypt_impl.h"

extern int nd_int();
extern unsigned char *init_arr();
extern RSA *init_rsa();
extern int init_padding();
extern void display(unsigned char *);

int main() {

  int flen = nd_int();
  const unsigned char *from = init_arr();
  unsigned char *to = init_arr();
  RSA *rsa = init_rsa();
  int padding = init_padding();

  rsa_ossl_private_encrypt(flen, from, to, rsa, padding);

  display(to);
  return 0;
}

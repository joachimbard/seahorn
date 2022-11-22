#include "openssl-bn.h"
#include "openssl-bn-aux.h"

// # ifndef OPENSSL_SMALL_FOOTPRINT
// #  define BN_MUL_COMBA
// #  define BN_SQR_COMBA
// #  define BN_RECURSION
// # endif

#include "openssl-bn_impl.h"
#include "openssl-bn_sqr_impl.h"
#include "openssl-bn_mul_impl.h"
#include "openssl-bn_mont_impl.h"

int main() {
//  BN_CTX *ctx = BN_CTX_new();
//  BIGNUM *r = BN_CTX_get(ctx);
//  BIGNUM *a = BN_CTX_get(ctx);

  BN_CTX *ctx = init_ctx();
  BIGNUM *r = init_bn(ctx);
  BIGNUM *a = init_bn(ctx);

  BN_sqr(r, a, ctx);

  // to avoid optimizations
  display_ctx(ctx);
  display_bn(r);
  display_bn(a);
  return 0;
}

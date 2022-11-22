#include "openssl-bn.h"
#include "openssl-bn-aux.h"

//#ifndef OPENSSL_SMALL_FOOTPRINT
//# define BN_MUL_COMBA
//# define BN_SQR_COMBA
//# define BN_RECURSION
//#endif

#include "openssl-bn_impl_no-ctx_no-err.h"
#include "openssl-bn_mul_impl.h"
#include "openssl-bn_sqr_impl.h"
#include "openssl-bn_mont_impl.h"

int main() {
// *version 1*
//  BN_CTX *ctx = BN_CTX_new();
//  // TODO: BN_CTX_get returns zero bignum
//  // better use init_bn() or search for something appropriate in the original repo
//  BIGNUM *r = BN_CTX_get(ctx);
//  BIGNUM *a = BN_CTX_get(ctx);
//  BIGNUM *b = BN_CTX_get(ctx);

// *version 2*
  BN_CTX *ctx = init_ctx();
  BIGNUM *r = init_bn(ctx);
  BIGNUM *a = init_bn(ctx);
  BIGNUM *b = init_bn(ctx);

  BN_mul(r, a, b, ctx);

  // to avoid optimizations
  display_ctx(ctx);
  display_bn(r);
  display_bn(a);
  display_bn(b);
  return 0;
}

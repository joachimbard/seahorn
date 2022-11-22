#include "openssl-bn.h"
#include "openssl-bn-aux.h"

#include "openssl-bn_impl.h"
#include "openssl-bn_exp_impl.h"
#include "openssl-bn_mul_impl.h"
#include "openssl-bn_sqr_impl.h"
#include "openssl-bn_mont_impl.h"

int main() {
//  BN_CTX *ctx = BN_CTX_new();
//  // TODO: BN_CTX_get returns zero bignum
//  // better use init_bn() or search for something appropriate in the original repo
//  BIGNUM *r = BN_CTX_get(ctx);
//  // init_bn(r);
//  BIGNUM *a = BN_CTX_get(ctx);
//  BIGNUM *p = BN_CTX_get(ctx);

  BN_CTX *ctx = init_ctx();
  BIGNUM *r = init_bn(ctx);
  BIGNUM *a = init_bn(ctx);
  BIGNUM *p = init_bn(ctx);

  BN_exp(r, a, p, ctx);

  // to avoid optimizations
  display_ctx(ctx);
  display_bn(r);
  display_bn(a);
  display_bn(p);
  return 0;
}

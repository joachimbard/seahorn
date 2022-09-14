#include <stddef.h>
#include <assert.h>
#include "openssl-bn.h"
#include "openssl-bn-aux.h"
#include <seahorn/seahorn.h>

#include "openssl-bn_mul_impl.h"
#include "openssl-bn_exp_impl.h"

int main() {
  BN_CTX *ctx = BN_CTX_new();
  // TODO: BN_CTX_get returns zero bignum
  // better use init_bn() or search for something appropriate in the original repo
  BIGNUM *r = BN_CTX_get(ctx);
  // BIGNUM *r = init_bn();
  BIGNUM *a = BN_CTX_get(ctx);
  BIGNUM *p = BN_CTX_get(ctx);

  BN_exp(r, a, p, ctx);

  // to avoid optimizations
  display_ctx(ctx);
  display_bn(r);
  display_bn(a);
  display_bn(p);
  return 0;
}

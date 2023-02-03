#include "openssl-bn.h"
#include "openssl-bn-aux.h"

#include "openssl-bn_mul_impl.h"

int main() {
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

#include <stddef.h>
#include <string.h>
#include "openssl-bn.h"
#include "openssl-bn-aux.h"
#include <seahorn/seahorn.h>

//#ifndef OPENSSL_SMALL_FOOTPRINT
//# define BN_MUL_COMBA
//# define BN_SQR_COMBA
//# define BN_RECURSION
//#endif

#include "openssl-bn_mul_impl.h"

int main() {
  BN_CTX *ctx = BN_CTX_new();
  // TODO: BN_CTX_get returns zero bignum
  // better use init_bn() or search for something appropriate in the original repo
  BIGNUM *r = BN_CTX_get(ctx);
  BIGNUM *a = BN_CTX_get(ctx);
  BIGNUM *b = BN_CTX_get(ctx);

  BN_mul(r, a, b, ctx);

  // to avoid optimizations
  display_ctx(ctx);
  display_bn(r);
  display_bn(a);
  display_bn(b);
  return 0;
}

#include "openssl-bn.h"
#include "openssl-bn-aux.h"

#include "openssl-bn_impl.h"
#include "openssl-bn_exp_impl.h"

int main(int argc, char **argv) {
  BN_CTX *ctx = BN_CTX_new();
  BIGNUM *r = BN_CTX_get(ctx);
  // TODO: BN_CTX_get returns zero bignum
  // better use init_bn() or search for something appropriate in the original repo
  // init_bn(a);
  // init_bn(p);
  // init_bn(m);
  BIGNUM *a = BN_CTX_get(ctx);
  BIGNUM *p = BN_CTX_get(ctx);
  BIGNUM *m = BN_CTX_get(ctx);
  BN_MONT_CTX *in_mont = NULL;

  start_clock();
  BN_mod_exp_mont_consttime(r, a, p, m, ctx, in_mont);
  end_clock();

  display_bn(r);
  return 0;
}

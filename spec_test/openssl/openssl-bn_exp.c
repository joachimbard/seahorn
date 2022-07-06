#include <stddef.h>
#include <assert.h>
#include "openssl.h"
#include <seahorn/seahorn.h>

//extern void __taint(int);
//extern void __is_tainted(int);
extern void display(void*);

#include "openssl-bn_mul_impl.h"
#include "openssl-bn_exp_impl.h"

int main() {
  BN_CTX *ctx = BN_CTX_new();
  BIGNUM *r = BN_CTX_get(ctx);
  BIGNUM *a = BN_CTX_get(ctx);
  BIGNUM *p = BN_CTX_get(ctx);

  BN_exp(r, a, p, ctx);

  // to avoid optimizations
  display(ctx);
  display(r);
  display(a);
  display(p);
  return 0;
}

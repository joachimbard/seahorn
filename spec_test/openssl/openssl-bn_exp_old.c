#include <stddef.h>
#include <assert.h>
#include "openssl.h"
#include <seahorn/seahorn.h>

extern void __taint(int);
extern void __is_tainted(int);
extern void display(void*);

# ifndef OPENSSL_SMALL_FOOTPRINT
#  define BN_MUL_COMBA
#  define BN_SQR_COMBA
#  define BN_RECURSION
# endif

// bn_sqr.c
int BN_sqr(BIGNUM *r, const BIGNUM *a, BN_CTX *ctx)
{
    int ret = bn_sqr_fixed_top(r, a, ctx);

    bn_correct_top(r);
    bn_check_top(r);

    return ret;
}

// TODO
#if 0
int bn_sqr_fixed_top(BIGNUM *r, const BIGNUM *a, BN_CTX *ctx)
{
    int max, al;
    int ret = 0;
    BIGNUM *tmp, *rr;

    bn_check_top(a);

    al = a->top;
    if (al <= 0) {
        r->top = 0;
        r->neg = 0;
        return 1;
    }

    BN_CTX_start(ctx);
    rr = (a != r) ? r : BN_CTX_get(ctx);
    tmp = BN_CTX_get(ctx);
    if (rr == NULL || tmp == NULL)
        goto err;

    max = 2 * al;               /* Non-zero (from above) */
    if (bn_wexpand(rr, max) == NULL)
        goto err;

    if (al == 4) {
#ifndef BN_SQR_COMBA
        BN_ULONG t[8];
        bn_sqr_normal(rr->d, a->d, 4, t);
#else
        bn_sqr_comba4(rr->d, a->d);
#endif
    } else if (al == 8) {
#ifndef BN_SQR_COMBA
        BN_ULONG t[16];
        bn_sqr_normal(rr->d, a->d, 8, t);
#else
        bn_sqr_comba8(rr->d, a->d);
#endif
    } else {
#if defined(BN_RECURSION)
        if (al < BN_SQR_RECURSIVE_SIZE_NORMAL) {
            BN_ULONG t[BN_SQR_RECURSIVE_SIZE_NORMAL * 2];
            bn_sqr_normal(rr->d, a->d, al, t);
        } else {
            int j, k;

            j = BN_num_bits_word((BN_ULONG)al);
            j = 1 << (j - 1);
            k = j + j;
            if (al == j) {
                if (bn_wexpand(tmp, k * 2) == NULL)
                    goto err;
                bn_sqr_recursive(rr->d, a->d, al, tmp->d);
            } else {
                if (bn_wexpand(tmp, max) == NULL)
                    goto err;
                bn_sqr_normal(rr->d, a->d, al, tmp->d);
            }
        }
#else
        if (bn_wexpand(tmp, max) == NULL)
            goto err;
        bn_sqr_normal(rr->d, a->d, al, tmp->d);
#endif
    }

    rr->neg = 0;
    rr->top = max;
    rr->flags |= BN_FLG_FIXED_TOP;
    if (r != rr && BN_copy(r, rr) == NULL)
        goto err;

    ret = 1;
 err:
    bn_check_top(rr);
    bn_check_top(tmp);
    BN_CTX_end(ctx);
    return ret;
}
#endif

// bn_mul.c
int BN_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx)
{
    int ret = bn_mul_fixed_top(r, a, b, ctx);

    bn_correct_top(r);
    bn_check_top(r);

    return ret;
}

int bn_mul_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx)
{
    int ret = 0;
    int top, al, bl;
    BIGNUM *rr;
#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
    int i;
#endif
#ifdef BN_RECURSION
    BIGNUM *t = NULL;
    int j = 0, k;
#endif

    bn_check_top(a);
    bn_check_top(b);
    bn_check_top(r);

    al = a->top;
    bl = b->top;

    if ((al == 0) || (bl == 0)) {
        BN_zero(r);
        return 1;
    }
    top = al + bl;

    BN_CTX_start(ctx);
    if ((r == a) || (r == b)) {
        if ((rr = BN_CTX_get(ctx)) == NULL)
            goto err;
    } else
        rr = r;

#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
    i = al - bl;
#endif
#ifdef BN_MUL_COMBA
    if (i == 0) {
# if 0
        if (al == 4) {
            if (bn_wexpand(rr, 8) == NULL)
                goto err;
            rr->top = 8;
            bn_mul_comba4(rr->d, a->d, b->d);
            goto end;
        }
# endif
        if (al == 8) {
            if (bn_wexpand(rr, 16) == NULL)
                goto err;
            rr->top = 16;
            bn_mul_comba8(rr->d, a->d, b->d);
            goto end;
        }
    }
#endif                          /* BN_MUL_COMBA */
#ifdef BN_RECURSION
    if ((al >= BN_MULL_SIZE_NORMAL) && (bl >= BN_MULL_SIZE_NORMAL)) {
        if (i >= -1 && i <= 1) {
            /*
             * Find out the power of two lower or equal to the longest of the
             * two numbers
             */
            if (i >= 0) {
                j = BN_num_bits_word((BN_ULONG)al);
            }
            if (i == -1) {
                j = BN_num_bits_word((BN_ULONG)bl);
            }
            j = 1 << (j - 1);
            assert(j <= al || j <= bl);
            k = j + j;
            t = BN_CTX_get(ctx);
            if (t == NULL)
                goto err;
            if (al > j || bl > j) {
                if (bn_wexpand(t, k * 4) == NULL)
                    goto err;
                if (bn_wexpand(rr, k * 4) == NULL)
                    goto err;
                bn_mul_part_recursive(rr->d, a->d, b->d,
                                      j, al - j, bl - j, t->d);
            } else {            /* al <= j || bl <= j */

                if (bn_wexpand(t, k * 2) == NULL)
                    goto err;
                if (bn_wexpand(rr, k * 2) == NULL)
                    goto err;
                bn_mul_recursive(rr->d, a->d, b->d, j, al - j, bl - j, t->d);
            }
            rr->top = top;
            goto end;
        }
    }
#endif                          /* BN_RECURSION */
    if (bn_wexpand(rr, top) == NULL)
        goto err;
    rr->top = top;
    bn_mul_normal(rr->d, a->d, al, b->d, bl);

#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
 end:
#endif
    rr->neg = a->neg ^ b->neg;
    rr->flags |= BN_FLG_FIXED_TOP;
    if (r != rr && BN_copy(r, rr) == NULL)
        goto err;

    ret = 1;
 err:
    bn_check_top(r);
    BN_CTX_end(ctx);
    return ret;
}

/* this one works - simple but works */
int BN_exp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p, BN_CTX *ctx)
{
    int i, bits, ret = 0;
    BIGNUM *v, *rr;

// XXX
//    if (BN_get_flags(p, BN_FLG_CONSTTIME) != 0
//            || BN_get_flags(a, BN_FLG_CONSTTIME) != 0) {
//        /* BN_FLG_CONSTTIME only supported by BN_mod_exp_mont() */
//        ERR_raise(ERR_LIB_BN, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
//        return 0;
//    }

    BN_CTX_start(ctx);
    rr = ((r == a) || (r == p)) ? BN_CTX_get(ctx) : r;
    v = BN_CTX_get(ctx);
    if (rr == NULL || v == NULL)
        goto err;

    if (BN_copy(v, a) == NULL)
        goto err;
    bits = BN_num_bits(p);

    if (BN_is_odd(p)) {
        if (BN_copy(rr, a) == NULL)
            goto err;
    } else {
        if (!BN_one(rr))
            goto err;
    }

    for (i = 1; i < bits; i++) {
        if (!BN_sqr(v, v, ctx))
            goto err;
        if (BN_is_bit_set(p, i)) {
            if (!BN_mul(rr, rr, v, ctx))
                goto err;
        }
    }
    if (r != rr && BN_copy(r, rr) == NULL)
        goto err;

    ret = 1;
 err:
    BN_CTX_end(ctx);
    bn_check_top(r);
    return ret;
}

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

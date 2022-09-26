#include <string.h>
#include <assert.h>
#include <limits.h>

#include "openssl-bn_ctx_impl.h"

// bn_lib.c
static BN_ULONG *bn_expand_internal(const BIGNUM *b, int words)
{
    BN_ULONG *a = NULL;

    if (words > (INT_MAX / (4 * BN_BITS2))) {
        ERR_raise(ERR_LIB_BN, BN_R_BIGNUM_TOO_LONG);
        return NULL;
    }
    if (BN_get_flags(b, BN_FLG_STATIC_DATA)) {
        ERR_raise(ERR_LIB_BN, BN_R_EXPAND_ON_STATIC_BIGNUM_DATA);
        return NULL;
    }
    if (BN_get_flags(b, BN_FLG_SECURE))
        a = OPENSSL_secure_zalloc(words * sizeof(*a));
    else
        a = OPENSSL_zalloc(words * sizeof(*a));
    if (a == NULL) {
        ERR_raise(ERR_LIB_BN, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    assert(b->top <= words);
    if (b->top > 0)
        memcpy(a, b->d, sizeof(*a) * b->top);

    return a;
}
BIGNUM *bn_expand2(BIGNUM *b, int words)
{
    if (words > b->dmax) {
        BN_ULONG *a = bn_expand_internal(b, words);
        if (!a)
            return NULL;
        if (b->d != NULL)
            bn_free_d(b, 1);
        b->d = a;
        b->dmax = words;
    }

    return b;
}
int BN_is_bit_set(const BIGNUM *a, int n)
{
    int i, j;

    bn_check_top(a);
    if (n < 0)
        return 0;
    i = n / BN_BITS2;
    j = n % BN_BITS2;
    if (a->top <= i)
        return 0;
    return (int)(((a->d[i]) >> j) & ((BN_ULONG)1));
}
int BN_is_odd(const BIGNUM *a)
{
    return (a->top > 0) && (a->d[0] & 1);
}
int BN_get_flags(const BIGNUM *b, int n)
{
    return b->flags & n;
}
BIGNUM *bn_wexpand(BIGNUM *a, int words)
{
    return (words <= a->dmax) ? a : bn_expand2(a, words);
}
void bn_correct_top(BIGNUM *a)
{
    BN_ULONG *ftl;
    int tmp_top = a->top;

    if (tmp_top > 0) {
        for (ftl = &(a->d[tmp_top]); tmp_top > 0; tmp_top--) {
            ftl--;
            if (*ftl != 0)
                break;
        }
        a->top = tmp_top;
    }
    if (a->top == 0)
        a->neg = 0;
    a->flags &= ~BN_FLG_FIXED_TOP;
    bn_pollute(a);
}

// crypto/mem.c
// XXX: with OPENSSL_NO_CRYPTO_MDEBUG
# define INCREMENT(x) /* empty */
# define FAILTEST() /* empty */

static int allow_customize = 1;
static CRYPTO_malloc_fn malloc_impl = CRYPTO_malloc;

void *CRYPTO_malloc(size_t num, const char *file, int line)
{
    INCREMENT(malloc_count);
    if (malloc_impl != CRYPTO_malloc)
        return malloc_impl(num, file, line);

    if (num == 0)
        return NULL;

    FAILTEST();
    if (allow_customize) {
        /*
         * Disallow customization after the first allocation. We only set this
         * if necessary to avoid a store to the same cache line on every
         * allocation.
         */
        allow_customize = 0;
    }

    return malloc(num);
}

void *CRYPTO_zalloc(size_t num, const char *file, int line)
{
    void *ret;

    ret = CRYPTO_malloc(num, file, line);
    FAILTEST();
    if (ret != NULL)
        memset(ret, 0, num);

    return ret;
}

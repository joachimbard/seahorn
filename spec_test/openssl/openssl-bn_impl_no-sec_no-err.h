#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>

//#define FIPS_MODULE
#define USE_RWLOCK

#include "openssl-bn_shift_impl.h"
#include "openssl-bn_add_impl.h"
#include "openssl-bn_ctx_impl.h"
#include "openssl-bn_lib_impl.h"
//#include "openssl-mem_sec_impl.h"
#include "openssl-mem_clr_impl.h"

//#include "my-err_impl.h"

// crypto/mem.c
// XXX: with OPENSSL_NO_CRYPTO_MDEBUG
# define INCREMENT(x) /* empty */
# define FAILTEST() /* empty */

static int allow_customize = 1;
static CRYPTO_malloc_fn malloc_impl = CRYPTO_malloc;
static CRYPTO_realloc_fn realloc_impl = CRYPTO_realloc;
static CRYPTO_free_fn free_impl = CRYPTO_free;

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

void *CRYPTO_realloc(void *str, size_t num, const char *file, int line)
{
    INCREMENT(realloc_count);
    if (realloc_impl != CRYPTO_realloc)
        return realloc_impl(str, num, file, line);

    FAILTEST();
    if (str == NULL)
        return CRYPTO_malloc(num, file, line);

    if (num == 0) {
        CRYPTO_free(str, file, line);
        return NULL;
    }

    return realloc(str, num);
}

void *CRYPTO_clear_realloc(void *str, size_t old_len, size_t num,
                           const char *file, int line)
{
    void *ret = NULL;

    if (str == NULL)
        return CRYPTO_malloc(num, file, line);

    if (num == 0) {
        CRYPTO_clear_free(str, old_len, file, line);
        return NULL;
    }

    /* Can't shrink the buffer since memcpy below copies |old_len| bytes. */
    if (num < old_len) {
        OPENSSL_cleanse((char*)str + num, old_len - num);
        return str;
    }

    ret = CRYPTO_malloc(num, file, line);
    if (ret != NULL) {
        memcpy(ret, str, old_len);
        CRYPTO_clear_free(str, old_len, file, line);
    }
    return ret;
}

void CRYPTO_free(void *str, const char *file, int line)
{
    INCREMENT(free_count);
    if (free_impl != CRYPTO_free) {
        free_impl(str, file, line);
        return;
    }

    free(str);
}

void CRYPTO_clear_free(void *str, size_t num, const char *file, int line)
{
    if (str == NULL)
        return;
    if (num)
        OPENSSL_cleanse(str, num);
    CRYPTO_free(str, file, line);
}

// crypto/cryptlib.c
void OPENSSL_showfatal(const char *fmta, ...)
{
#ifndef OPENSSL_NO_STDIO
    va_list ap;

    va_start(ap, fmta);
    vfprintf(stderr, fmta, ap);
    va_end(ap);
#endif
}

int OPENSSL_isservice(void)
{
    return 0;
}

void OPENSSL_die(const char *message, const char *file, int line)
{
    OPENSSL_showfatal("%s:%d: OpenSSL internal error: %s\n",
                      file, line, message);
#if !defined(_WIN32)
    abort();
#else
    /*
     * Win32 abort() customarily shows a dialog, but we just did that...
     */
# if !defined(_WIN32_WCE)
    raise(SIGABRT);
# endif
    _exit(3);
#endif
}


// crypto/threads_pthread.c
//#  include <sys/types.h>
//#  include <unistd.h>

CRYPTO_RWLOCK *CRYPTO_THREAD_lock_new(void)
{
# ifdef USE_RWLOCK
    CRYPTO_RWLOCK *lock;

    if ((lock = OPENSSL_zalloc(sizeof(pthread_rwlock_t))) == NULL) {
        /* Don't set error, to avoid recursion blowup. */
        return NULL;
    }

    if (pthread_rwlock_init(lock, NULL) != 0) {
        OPENSSL_free(lock);
        return NULL;
    }
# else
    pthread_mutexattr_t attr;
    CRYPTO_RWLOCK *lock;

    if ((lock = OPENSSL_zalloc(sizeof(pthread_mutex_t))) == NULL) {
        /* Don't set error, to avoid recursion blowup. */
        return NULL;
    }

    /*
     * We don't use recursive mutexes, but try to catch errors if we do.
     */
    pthread_mutexattr_init(&attr);
#  if !defined (__TANDEM) && !defined (_SPT_MODEL_)
#   if !defined(NDEBUG) && !defined(OPENSSL_NO_MUTEX_ERRORCHECK)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#   else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#   endif
#  else
    /* The SPT Thread Library does not define MUTEX attributes. */
#  endif

    if (pthread_mutex_init(lock, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        OPENSSL_free(lock);
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);
# endif

    return lock;
}

__owur int CRYPTO_THREAD_read_lock(CRYPTO_RWLOCK *lock)
{
# ifdef USE_RWLOCK
    if (pthread_rwlock_rdlock(lock) != 0)
        return 0;
# else
    if (pthread_mutex_lock(lock) != 0) {
        assert(errno != EDEADLK && errno != EBUSY);
        return 0;
    }
# endif

    return 1;
}

__owur int CRYPTO_THREAD_write_lock(CRYPTO_RWLOCK *lock)
{
# ifdef USE_RWLOCK
    if (pthread_rwlock_wrlock(lock) != 0)
        return 0;
# else
    if (pthread_mutex_lock(lock) != 0) {
        assert(errno != EDEADLK && errno != EBUSY);
        return 0;
    }
# endif

    return 1;
}

int CRYPTO_THREAD_unlock(CRYPTO_RWLOCK *lock)
{
# ifdef USE_RWLOCK
    if (pthread_rwlock_unlock(lock) != 0)
        return 0;
# else
    if (pthread_mutex_unlock(lock) != 0) {
        assert(errno != EPERM);
        return 0;
    }
# endif

    return 1;
}

void CRYPTO_THREAD_lock_free(CRYPTO_RWLOCK *lock)
{
    if (lock == NULL)
        return;

# ifdef USE_RWLOCK
    pthread_rwlock_destroy(lock);
# else
    pthread_mutex_destroy(lock);
# endif
    OPENSSL_free(lock);

    return;
}


// bn/bn_asm.c
// with !(defined(BN_LLONG) || defined(BN_UMULT_HIGH))
// with !BN_LLONG
BN_ULONG bn_mul_add_words(BN_ULONG *rp, const BN_ULONG *ap, int num,
                          BN_ULONG w)
{
    BN_ULONG c = 0;
    BN_ULONG bl, bh;

    assert(num >= 0);
    if (num <= 0)
        return (BN_ULONG)0;

    bl = LBITS(w);
    bh = HBITS(w);

# ifndef OPENSSL_SMALL_FOOTPRINT
    while (num & ~3) {
        mul_add(rp[0], ap[0], bl, bh, c);
        mul_add(rp[1], ap[1], bl, bh, c);
        mul_add(rp[2], ap[2], bl, bh, c);
        mul_add(rp[3], ap[3], bl, bh, c);
        ap += 4;
        rp += 4;
        num -= 4;
    }
# endif
    while (num) {
        mul_add(rp[0], ap[0], bl, bh, c);
        ap++;
        rp++;
        num--;
    }
    return c;
}

BN_ULONG bn_mul_words(BN_ULONG *rp, const BN_ULONG *ap, int num, BN_ULONG w)
{
    BN_ULONG carry = 0;
    BN_ULONG bl, bh;

    assert(num >= 0);
    if (num <= 0)
        return (BN_ULONG)0;

    bl = LBITS(w);
    bh = HBITS(w);

# ifndef OPENSSL_SMALL_FOOTPRINT
    while (num & ~3) {
        mul(rp[0], ap[0], bl, bh, carry);
        mul(rp[1], ap[1], bl, bh, carry);
        mul(rp[2], ap[2], bl, bh, carry);
        mul(rp[3], ap[3], bl, bh, carry);
        ap += 4;
        rp += 4;
        num -= 4;
    }
# endif
    while (num) {
        mul(rp[0], ap[0], bl, bh, carry);
        ap++;
        rp++;
        num--;
    }
    return carry;
}

void bn_sqr_words(BN_ULONG *r, const BN_ULONG *a, int n)
{
    assert(n >= 0);
    if (n <= 0)
        return;

# ifndef OPENSSL_SMALL_FOOTPRINT
    while (n & ~3) {
        sqr64(r[0], r[1], a[0]);
        sqr64(r[2], r[3], a[1]);
        sqr64(r[4], r[5], a[2]);
        sqr64(r[6], r[7], a[3]);
        a += 4;
        r += 8;
        n -= 4;
    }
# endif
    while (n) {
        sqr64(r[0], r[1], a[0]);
        a++;
        r += 2;
        n--;
    }
}

BN_ULONG bn_div_words(BN_ULONG h, BN_ULONG l, BN_ULONG d)
{
    BN_ULONG dh, dl, q, ret = 0, th, tl, t;
    int i, count = 2;

    if (d == 0)
        return BN_MASK2;

    i = BN_num_bits_word(d);
    assert((i == BN_BITS2) || (h <= (BN_ULONG)1 << i));

    i = BN_BITS2 - i;
    if (h >= d)
        h -= d;

    if (i) {
        d <<= i;
        h = (h << i) | (l >> (BN_BITS2 - i));
        l <<= i;
    }
    dh = (d & BN_MASK2h) >> BN_BITS4;
    dl = (d & BN_MASK2l);
    for (;;) {
        if ((h >> BN_BITS4) == dh)
            q = BN_MASK2l;
        else
            q = h / dh;

        th = q * dh;
        tl = dl * q;
        for (;;) {
            t = h - th;
            if ((t & BN_MASK2h) ||
                ((tl) <= ((t << BN_BITS4) | ((l & BN_MASK2h) >> BN_BITS4))))
                break;
            q--;
            th -= dh;
            tl -= dl;
        }
        t = (tl >> BN_BITS4);
        tl = (tl << BN_BITS4) & BN_MASK2h;
        th += t;

        if (l < tl)
            th++;
        l -= tl;
        if (h < th) {
            h += d;
            q--;
        }
        h -= th;

        if (--count == 0)
            break;

        ret = q << BN_BITS4;
        h = ((h << BN_BITS4) | (l >> BN_BITS4)) & BN_MASK2;
        l = (l & BN_MASK2l) << BN_BITS4;
    }
    ret |= q;
    return ret;
}

BN_ULONG bn_add_words(BN_ULONG *r, const BN_ULONG *a, const BN_ULONG *b,
                      int n)
{
    BN_ULONG c, l, t;

    assert(n >= 0);
    if (n <= 0)
        return (BN_ULONG)0;

    c = 0;
# ifndef OPENSSL_SMALL_FOOTPRINT
    while (n & ~3) {
        t = a[0];
        t = (t + c) & BN_MASK2;
        c = (t < c);
        l = (t + b[0]) & BN_MASK2;
        c += (l < t);
        r[0] = l;
        t = a[1];
        t = (t + c) & BN_MASK2;
        c = (t < c);
        l = (t + b[1]) & BN_MASK2;
        c += (l < t);
        r[1] = l;
        t = a[2];
        t = (t + c) & BN_MASK2;
        c = (t < c);
        l = (t + b[2]) & BN_MASK2;
        c += (l < t);
        r[2] = l;
        t = a[3];
        t = (t + c) & BN_MASK2;
        c = (t < c);
        l = (t + b[3]) & BN_MASK2;
        c += (l < t);
        r[3] = l;
        a += 4;
        b += 4;
        r += 4;
        n -= 4;
    }
# endif
    while (n) {
        t = a[0];
        t = (t + c) & BN_MASK2;
        c = (t < c);
        l = (t + b[0]) & BN_MASK2;
        c += (l < t);
        r[0] = l;
        a++;
        b++;
        r++;
        n--;
    }
    return (BN_ULONG)c;
}

BN_ULONG bn_sub_words(BN_ULONG *r, const BN_ULONG *a, const BN_ULONG *b,
                      int n)
{
    BN_ULONG t1, t2;
    int c = 0;

    assert(n >= 0);
    if (n <= 0)
        return (BN_ULONG)0;

#ifndef OPENSSL_SMALL_FOOTPRINT
    while (n & ~3) {
        t1 = a[0];
        t2 = b[0];
        r[0] = (t1 - t2 - c) & BN_MASK2;
        if (t1 != t2)
            c = (t1 < t2);
        t1 = a[1];
        t2 = b[1];
        r[1] = (t1 - t2 - c) & BN_MASK2;
        if (t1 != t2)
            c = (t1 < t2);
        t1 = a[2];
        t2 = b[2];
        r[2] = (t1 - t2 - c) & BN_MASK2;
        if (t1 != t2)
            c = (t1 < t2);
        t1 = a[3];
        t2 = b[3];
        r[3] = (t1 - t2 - c) & BN_MASK2;
        if (t1 != t2)
            c = (t1 < t2);
        a += 4;
        b += 4;
        r += 4;
        n -= 4;
    }
#endif
    while (n) {
        t1 = a[0];
        t2 = b[0];
        r[0] = (t1 - t2 - c) & BN_MASK2;
        if (t1 != t2)
            c = (t1 < t2);
        a++;
        b++;
        r++;
        n--;
    }
    return c;
}


// crypto/bn/bn_gcd.c
/*
 * bn_mod_inverse_no_branch is a special version of BN_mod_inverse. It does
 * not contain branches that may leak sensitive information.
 *
 * This is a static function, we ensure all callers in this file pass valid
 * arguments: all passed pointers here are non-NULL.
 */
static ossl_inline
BIGNUM *bn_mod_inverse_no_branch(BIGNUM *in,
                                 const BIGNUM *a, const BIGNUM *n,
                                 BN_CTX *ctx, int *pnoinv)
{
    BIGNUM *A, *B, *X, *Y, *M, *D, *T, *R = NULL;
    BIGNUM *ret = NULL;
    int sign;

    bn_check_top(a);
    bn_check_top(n);

    BN_CTX_start(ctx);
    A = BN_CTX_get(ctx);
    B = BN_CTX_get(ctx);
    X = BN_CTX_get(ctx);
    D = BN_CTX_get(ctx);
    M = BN_CTX_get(ctx);
    Y = BN_CTX_get(ctx);
    T = BN_CTX_get(ctx);
    if (T == NULL)
        goto err;

    if (in == NULL)
        R = BN_new();
    else
        R = in;
    if (R == NULL)
        goto err;

    BN_one(X);
    BN_zero(Y);
    if (BN_copy(B, a) == NULL)
        goto err;
    if (BN_copy(A, n) == NULL)
        goto err;
    A->neg = 0;

    if (B->neg || (BN_ucmp(B, A) >= 0)) {
        /*
         * Turn BN_FLG_CONSTTIME flag on, so that when BN_div is invoked,
         * BN_div_no_branch will be called eventually.
         */
         {
            BIGNUM local_B;
            bn_init(&local_B);
            BN_with_flags(&local_B, B, BN_FLG_CONSTTIME);
            if (!BN_nnmod(B, &local_B, A, ctx))
                goto err;
            /* Ensure local_B goes out of scope before any further use of B */
        }
    }
    sign = -1;
    /*-
     * From  B = a mod |n|,  A = |n|  it follows that
     *
     *      0 <= B < A,
     *     -sign*X*a  ==  B   (mod |n|),
     *      sign*Y*a  ==  A   (mod |n|).
     */

    while (!BN_is_zero(B)) {
        BIGNUM *tmp;

        /*-
         *      0 < B < A,
         * (*) -sign*X*a  ==  B   (mod |n|),
         *      sign*Y*a  ==  A   (mod |n|)
         */

        /*
         * Turn BN_FLG_CONSTTIME flag on, so that when BN_div is invoked,
         * BN_div_no_branch will be called eventually.
         */
        {
            BIGNUM local_A;
            bn_init(&local_A);
            BN_with_flags(&local_A, A, BN_FLG_CONSTTIME);

            /* (D, M) := (A/B, A%B) ... */
            if (!BN_div(D, M, &local_A, B, ctx))
                goto err;
            /* Ensure local_A goes out of scope before any further use of A */
        }

        /*-
         * Now
         *      A = D*B + M;
         * thus we have
         * (**)  sign*Y*a  ==  D*B + M   (mod |n|).
         */

        tmp = A;                /* keep the BIGNUM object, the value does not
                                 * matter */

        /* (A, B) := (B, A mod B) ... */
        A = B;
        B = M;
        /* ... so we have  0 <= B < A  again */

        /*-
         * Since the former  M  is now  B  and the former  B  is now  A,
         * (**) translates into
         *       sign*Y*a  ==  D*A + B    (mod |n|),
         * i.e.
         *       sign*Y*a - D*A  ==  B    (mod |n|).
         * Similarly, (*) translates into
         *      -sign*X*a  ==  A          (mod |n|).
         *
         * Thus,
         *   sign*Y*a + D*sign*X*a  ==  B  (mod |n|),
         * i.e.
         *        sign*(Y + D*X)*a  ==  B  (mod |n|).
         *
         * So if we set  (X, Y, sign) := (Y + D*X, X, -sign), we arrive back at
         *      -sign*X*a  ==  B   (mod |n|),
         *       sign*Y*a  ==  A   (mod |n|).
         * Note that  X  and  Y  stay non-negative all the time.
         */

        if (!BN_mul(tmp, D, X, ctx))
            goto err;
        if (!BN_add(tmp, tmp, Y))
            goto err;

        M = Y;                  /* keep the BIGNUM object, the value does not
                                 * matter */
        Y = X;
        X = tmp;
        sign = -sign;
    }

    /*-
     * The while loop (Euclid's algorithm) ends when
     *      A == gcd(a,n);
     * we have
     *       sign*Y*a  ==  A  (mod |n|),
     * where  Y  is non-negative.
     */

    if (sign < 0) {
        if (!BN_sub(Y, n, Y))
            goto err;
    }
    /* Now  Y*a  ==  A  (mod |n|).  */

    if (BN_is_one(A)) {
        /* Y*a == 1  (mod |n|) */
        if (!Y->neg && BN_ucmp(Y, n) < 0) {
            if (!BN_copy(R, Y))
                goto err;
        } else {
            if (!BN_nnmod(R, Y, n, ctx))
                goto err;
        }
    } else {
        *pnoinv = 1;
        /* caller sets the BN_R_NO_INVERSE error */
        goto err;
    }

    ret = R;
    *pnoinv = 0;

 err:
    if ((ret == NULL) && (in == NULL))
        BN_free(R);
    BN_CTX_end(ctx);
    bn_check_top(ret);
    return ret;
}

BIGNUM *int_bn_mod_inverse(BIGNUM *in,
                           const BIGNUM *a, const BIGNUM *n, BN_CTX *ctx,
                           int *pnoinv)
{
    BIGNUM *A, *B, *X, *Y, *M, *D, *T, *R = NULL;
    BIGNUM *ret = NULL;
    int sign;

    /* This is invalid input so we don't worry about constant time here */
    if (BN_abs_is_word(n, 1) || BN_is_zero(n)) {
        *pnoinv = 1;
        return NULL;
    }

    *pnoinv = 0;

    if ((BN_get_flags(a, BN_FLG_CONSTTIME) != 0)
        || (BN_get_flags(n, BN_FLG_CONSTTIME) != 0)) {
        return bn_mod_inverse_no_branch(in, a, n, ctx, pnoinv);
    }

    bn_check_top(a);
    bn_check_top(n);

    BN_CTX_start(ctx);
    A = BN_CTX_get(ctx);
    B = BN_CTX_get(ctx);
    X = BN_CTX_get(ctx);
    D = BN_CTX_get(ctx);
    M = BN_CTX_get(ctx);
    Y = BN_CTX_get(ctx);
    T = BN_CTX_get(ctx);
    if (T == NULL)
        goto err;

    if (in == NULL)
        R = BN_new();
    else
        R = in;
    if (R == NULL)
        goto err;

    BN_one(X);
    BN_zero(Y);
    if (BN_copy(B, a) == NULL)
        goto err;
    if (BN_copy(A, n) == NULL)
        goto err;
    A->neg = 0;
    if (B->neg || (BN_ucmp(B, A) >= 0)) {
        if (!BN_nnmod(B, B, A, ctx))
            goto err;
    }
    sign = -1;
    /*-
     * From  B = a mod |n|,  A = |n|  it follows that
     *
     *      0 <= B < A,
     *     -sign*X*a  ==  B   (mod |n|),
     *      sign*Y*a  ==  A   (mod |n|).
     */

    if (BN_is_odd(n) && (BN_num_bits(n) <= 2048)) {
        /*
         * Binary inversion algorithm; requires odd modulus. This is faster
         * than the general algorithm if the modulus is sufficiently small
         * (about 400 .. 500 bits on 32-bit systems, but much more on 64-bit
         * systems)
         */
        int shift;

        while (!BN_is_zero(B)) {
            /*-
             *      0 < B < |n|,
             *      0 < A <= |n|,
             * (1) -sign*X*a  ==  B   (mod |n|),
             * (2)  sign*Y*a  ==  A   (mod |n|)
             */

            /*
             * Now divide B by the maximum possible power of two in the
             * integers, and divide X by the same value mod |n|. When we're
             * done, (1) still holds.
             */
            shift = 0;
            while (!BN_is_bit_set(B, shift)) { /* note that 0 < B */
                shift++;

                if (BN_is_odd(X)) {
                    if (!BN_uadd(X, X, n))
                        goto err;
                }
                /*
                 * now X is even, so we can easily divide it by two
                 */
                if (!BN_rshift1(X, X))
                    goto err;
            }
            if (shift > 0) {
                if (!BN_rshift(B, B, shift))
                    goto err;
            }

            /*
             * Same for A and Y.  Afterwards, (2) still holds.
             */
            shift = 0;
            while (!BN_is_bit_set(A, shift)) { /* note that 0 < A */
                shift++;

                if (BN_is_odd(Y)) {
                    if (!BN_uadd(Y, Y, n))
                        goto err;
                }
                /* now Y is even */
                if (!BN_rshift1(Y, Y))
                    goto err;
            }
            if (shift > 0) {
                if (!BN_rshift(A, A, shift))
                    goto err;
            }

            /*-
             * We still have (1) and (2).
             * Both  A  and  B  are odd.
             * The following computations ensure that
             *
             *     0 <= B < |n|,
             *      0 < A < |n|,
             * (1) -sign*X*a  ==  B   (mod |n|),
             * (2)  sign*Y*a  ==  A   (mod |n|),
             *
             * and that either  A  or  B  is even in the next iteration.
             */
            if (BN_ucmp(B, A) >= 0) {
                /* -sign*(X + Y)*a == B - A  (mod |n|) */
                if (!BN_uadd(X, X, Y))
                    goto err;
                /*
                 * NB: we could use BN_mod_add_quick(X, X, Y, n), but that
                 * actually makes the algorithm slower
                 */
                if (!BN_usub(B, B, A))
                    goto err;
            } else {
                /*  sign*(X + Y)*a == A - B  (mod |n|) */
                if (!BN_uadd(Y, Y, X))
                    goto err;
                /*
                 * as above, BN_mod_add_quick(Y, Y, X, n) would slow things down
                 */
                if (!BN_usub(A, A, B))
                    goto err;
            }
        }
    } else {
        /* general inversion algorithm */

        while (!BN_is_zero(B)) {
            BIGNUM *tmp;

            /*-
             *      0 < B < A,
             * (*) -sign*X*a  ==  B   (mod |n|),
             *      sign*Y*a  ==  A   (mod |n|)
             */

            /* (D, M) := (A/B, A%B) ... */
            if (BN_num_bits(A) == BN_num_bits(B)) {
                if (!BN_one(D))
                    goto err;
                if (!BN_sub(M, A, B))
                    goto err;
            } else if (BN_num_bits(A) == BN_num_bits(B) + 1) {
                /* A/B is 1, 2, or 3 */
                if (!BN_lshift1(T, B))
                    goto err;
                if (BN_ucmp(A, T) < 0) {
                    /* A < 2*B, so D=1 */
                    if (!BN_one(D))
                        goto err;
                    if (!BN_sub(M, A, B))
                        goto err;
                } else {
                    /* A >= 2*B, so D=2 or D=3 */
                    if (!BN_sub(M, A, T))
                        goto err;
                    if (!BN_add(D, T, B))
                        goto err; /* use D (:= 3*B) as temp */
                    if (BN_ucmp(A, D) < 0) {
                        /* A < 3*B, so D=2 */
                        if (!BN_set_word(D, 2))
                            goto err;
                        /*
                         * M (= A - 2*B) already has the correct value
                         */
                    } else {
                        /* only D=3 remains */
                        if (!BN_set_word(D, 3))
                            goto err;
                        /*
                         * currently M = A - 2*B, but we need M = A - 3*B
                         */
                        if (!BN_sub(M, M, B))
                            goto err;
                    }
                }
            } else {
                if (!BN_div(D, M, A, B, ctx))
                    goto err;
            }

            /*-
             * Now
             *      A = D*B + M;
             * thus we have
             * (**)  sign*Y*a  ==  D*B + M   (mod |n|).
             */

            tmp = A;    /* keep the BIGNUM object, the value does not matter */

            /* (A, B) := (B, A mod B) ... */
            A = B;
            B = M;
            /* ... so we have  0 <= B < A  again */

            /*-
             * Since the former  M  is now  B  and the former  B  is now  A,
             * (**) translates into
             *       sign*Y*a  ==  D*A + B    (mod |n|),
             * i.e.
             *       sign*Y*a - D*A  ==  B    (mod |n|).
             * Similarly, (*) translates into
             *      -sign*X*a  ==  A          (mod |n|).
             *
             * Thus,
             *   sign*Y*a + D*sign*X*a  ==  B  (mod |n|),
             * i.e.
             *        sign*(Y + D*X)*a  ==  B  (mod |n|).
             *
             * So if we set  (X, Y, sign) := (Y + D*X, X, -sign), we arrive back at
             *      -sign*X*a  ==  B   (mod |n|),
             *       sign*Y*a  ==  A   (mod |n|).
             * Note that  X  and  Y  stay non-negative all the time.
             */

            /*
             * most of the time D is very small, so we can optimize tmp := D*X+Y
             */
            if (BN_is_one(D)) {
                if (!BN_add(tmp, X, Y))
                    goto err;
            } else {
                if (BN_is_word(D, 2)) {
                    if (!BN_lshift1(tmp, X))
                        goto err;
                } else if (BN_is_word(D, 4)) {
                    if (!BN_lshift(tmp, X, 2))
                        goto err;
                } else if (D->top == 1) {
                    if (!BN_copy(tmp, X))
                        goto err;
                    if (!BN_mul_word(tmp, D->d[0]))
                        goto err;
                } else {
                    if (!BN_mul(tmp, D, X, ctx))
                        goto err;
                }
                if (!BN_add(tmp, tmp, Y))
                    goto err;
            }

            M = Y;      /* keep the BIGNUM object, the value does not matter */
            Y = X;
            X = tmp;
            sign = -sign;
        }
    }

    /*-
     * The while loop (Euclid's algorithm) ends when
     *      A == gcd(a,n);
     * we have
     *       sign*Y*a  ==  A  (mod |n|),
     * where  Y  is non-negative.
     */

    if (sign < 0) {
        if (!BN_sub(Y, n, Y))
            goto err;
    }
    /* Now  Y*a  ==  A  (mod |n|).  */

    if (BN_is_one(A)) {
        /* Y*a == 1  (mod |n|) */
        if (!Y->neg && BN_ucmp(Y, n) < 0) {
            if (!BN_copy(R, Y))
                goto err;
        } else {
            if (!BN_nnmod(R, Y, n, ctx))
                goto err;
        }
    } else {
        *pnoinv = 1;
        goto err;
    }
    ret = R;
 err:
    if ((ret == NULL) && (in == NULL))
        BN_free(R);
    BN_CTX_end(ctx);
    bn_check_top(ret);
    return ret;
}

/* solves ax == 1 (mod n) */
BIGNUM *BN_mod_inverse(BIGNUM *in,
                       const BIGNUM *a, const BIGNUM *n, BN_CTX *ctx)
{
    BN_CTX *new_ctx = NULL;
    BIGNUM *rv;
    int noinv = 0;

    if (ctx == NULL) {
        ctx = new_ctx = BN_CTX_new_ex(NULL);
        if (ctx == NULL) {
            ERR_raise(ERR_LIB_BN, ERR_R_MALLOC_FAILURE);
            return NULL;
        }
    }

    rv = int_bn_mod_inverse(in, a, n, ctx, &noinv);
    if (noinv)
        ERR_raise(ERR_LIB_BN, BN_R_NO_INVERSE);
    BN_CTX_free(new_ctx);
    return rv;
}


// crypto/bn_bn_div.c
static int bn_left_align(BIGNUM *num)
{
    BN_ULONG *d = num->d, n, m, rmask;
    int top = num->top;
    int rshift = BN_num_bits_word(d[top - 1]), lshift, i;

    lshift = BN_BITS2 - rshift;
    rshift %= BN_BITS2;            /* say no to undefined behaviour */
    rmask = (BN_ULONG)0 - rshift;  /* rmask = 0 - (rshift != 0) */
    rmask |= rmask >> 8;

    for (i = 0, m = 0; i < top; i++) {
        n = d[i];
        d[i] = ((n << lshift) | m) & BN_MASK2;
        m = (n >> rshift) & rmask;
    }

    return lshift;
}

int BN_div(BIGNUM *dv, BIGNUM *rm, const BIGNUM *num, const BIGNUM *divisor,
           BN_CTX *ctx)
{
    int ret;

    if (BN_is_zero(divisor)) {
        ERR_raise(ERR_LIB_BN, BN_R_DIV_BY_ZERO);
        return 0;
    }

    /*
     * Invalid zero-padding would have particularly bad consequences so don't
     * just rely on bn_check_top() here (bn_check_top() works only for
     * BN_DEBUG builds)
     */
    if (divisor->d[divisor->top - 1] == 0) {
        ERR_raise(ERR_LIB_BN, BN_R_NOT_INITIALIZED);
        return 0;
    }

    ret = bn_div_fixed_top(dv, rm, num, divisor, ctx);

    if (ret) {
        if (dv != NULL)
            bn_correct_top(dv);
        if (rm != NULL)
            bn_correct_top(rm);
    }

    return ret;
}

int bn_div_fixed_top(BIGNUM *dv, BIGNUM *rm, const BIGNUM *num,
                     const BIGNUM *divisor, BN_CTX *ctx)
{
    int norm_shift, i, j, loop;
    BIGNUM *tmp, *snum, *sdiv, *res;
    BN_ULONG *resp, *wnum, *wnumtop;
    BN_ULONG d0, d1;
    int num_n, div_n, num_neg;

    assert(divisor->top > 0 && divisor->d[divisor->top - 1] != 0);

    bn_check_top(num);
    bn_check_top(divisor);
    bn_check_top(dv);
    bn_check_top(rm);

    BN_CTX_start(ctx);
    res = (dv == NULL) ? BN_CTX_get(ctx) : dv;
    tmp = BN_CTX_get(ctx);
    snum = BN_CTX_get(ctx);
    sdiv = BN_CTX_get(ctx);
    if (sdiv == NULL)
        goto err;

    /* First we normalise the numbers */
    if (!BN_copy(sdiv, divisor))
        goto err;
    norm_shift = bn_left_align(sdiv);
    sdiv->neg = 0;
    /*
     * Note that bn_lshift_fixed_top's output is always one limb longer
     * than input, even when norm_shift is zero. This means that amount of
     * inner loop iterations is invariant of dividend value, and that one
     * doesn't need to compare dividend and divisor if they were originally
     * of the same bit length.
     */
    if (!(bn_lshift_fixed_top(snum, num, norm_shift)))
        goto err;

    div_n = sdiv->top;
    num_n = snum->top;

    if (num_n <= div_n) {
        /* caller didn't pad dividend -> no constant-time guarantee... */
        if (bn_wexpand(snum, div_n + 1) == NULL)
            goto err;
        memset(&(snum->d[num_n]), 0, (div_n - num_n + 1) * sizeof(BN_ULONG));
        snum->top = num_n = div_n + 1;
    }

    loop = num_n - div_n;
    /*
     * Lets setup a 'window' into snum This is the part that corresponds to
     * the current 'area' being divided
     */
    wnum = &(snum->d[loop]);
    wnumtop = &(snum->d[num_n - 1]);

    /* Get the top 2 words of sdiv */
    d0 = sdiv->d[div_n - 1];
    d1 = (div_n == 1) ? 0 : sdiv->d[div_n - 2];

    /* Setup quotient */
    if (!bn_wexpand(res, loop))
        goto err;
    num_neg = num->neg;
    res->neg = (num_neg ^ divisor->neg);
    res->top = loop;
    res->flags |= BN_FLG_FIXED_TOP;
    resp = &(res->d[loop]);

    /* space for temp */
    if (!bn_wexpand(tmp, (div_n + 1)))
        goto err;

    for (i = 0; i < loop; i++, wnumtop--) {
        BN_ULONG q, l0;
        /*
         * the first part of the loop uses the top two words of snum and sdiv
         * to calculate a BN_ULONG q such that | wnum - sdiv * q | < sdiv
         */
# if defined(BN_DIV3W)
        q = bn_div_3_words(wnumtop, d1, d0);
# else
        BN_ULONG n0, n1, rem = 0;

        n0 = wnumtop[0];
        n1 = wnumtop[-1];
        if (n0 == d0)
            q = BN_MASK2;
        else {                  /* n0 < d0 */
            BN_ULONG n2 = (wnumtop == wnum) ? 0 : wnumtop[-2];
#  ifdef BN_LLONG
            BN_ULLONG t2;

#   if defined(BN_LLONG) && defined(BN_DIV2W) && !defined(bn_div_words)
            q = (BN_ULONG)(((((BN_ULLONG) n0) << BN_BITS2) | n1) / d0);
#   else
            q = bn_div_words(n0, n1, d0);
#   endif

#   ifndef REMAINDER_IS_ALREADY_CALCULATED
            /*
             * rem doesn't have to be BN_ULLONG. The least we
             * know it's less that d0, isn't it?
             */
            rem = (n1 - q * d0) & BN_MASK2;
#   endif
            t2 = (BN_ULLONG) d1 *q;

            for (;;) {
                if (t2 <= ((((BN_ULLONG) rem) << BN_BITS2) | n2))
                    break;
                q--;
                rem += d0;
                if (rem < d0)
                    break;      /* don't let rem overflow */
                t2 -= d1;
            }
#  else                         /* !BN_LLONG */
            BN_ULONG t2l, t2h;

            q = bn_div_words(n0, n1, d0);
#   ifndef REMAINDER_IS_ALREADY_CALCULATED
            rem = (n1 - q * d0) & BN_MASK2;
#   endif

#   if defined(BN_UMULT_LOHI)
            BN_UMULT_LOHI(t2l, t2h, d1, q);
#   elif defined(BN_UMULT_HIGH)
            t2l = d1 * q;
            t2h = BN_UMULT_HIGH(d1, q);
#   else
            {
                BN_ULONG ql, qh;
                t2l = LBITS(d1);
                t2h = HBITS(d1);
                ql = LBITS(q);
                qh = HBITS(q);
                mul64(t2l, t2h, ql, qh); /* t2=(BN_ULLONG)d1*q; */
            }
#   endif

            for (;;) {
                if ((t2h < rem) || ((t2h == rem) && (t2l <= n2)))
                    break;
                q--;
                rem += d0;
                if (rem < d0)
                    break;      /* don't let rem overflow */
                if (t2l < d1)
                    t2h--;
                t2l -= d1;
            }
#  endif                        /* !BN_LLONG */
        }
# endif                         /* !BN_DIV3W */

        l0 = bn_mul_words(tmp->d, sdiv->d, div_n, q);
        tmp->d[div_n] = l0;
        wnum--;
        /*
         * ignore top values of the bignums just sub the two BN_ULONG arrays
         * with bn_sub_words
         */
        l0 = bn_sub_words(wnum, wnum, tmp->d, div_n + 1);
        q -= l0;
        /*
         * Note: As we have considered only the leading two BN_ULONGs in
         * the calculation of q, sdiv * q might be greater than wnum (but
         * then (q-1) * sdiv is less or equal than wnum)
         */
        for (l0 = 0 - l0, j = 0; j < div_n; j++)
            tmp->d[j] = sdiv->d[j] & l0;
        l0 = bn_add_words(wnum, wnum, tmp->d, div_n);
        (*wnumtop) += l0;
        assert((*wnumtop) == 0);

        /* store part of the result */
        *--resp = q;
    }
    /* snum holds remainder, it's as wide as divisor */
    snum->neg = num_neg;
    snum->top = div_n;
    snum->flags |= BN_FLG_FIXED_TOP;
    if (rm != NULL)
        bn_rshift_fixed_top(rm, snum, norm_shift);
    BN_CTX_end(ctx);
    return 1;
 err:
    bn_check_top(rm);
    BN_CTX_end(ctx);
    return 0;
}


// crypto/bn/bn_mod.c
int BN_nnmod(BIGNUM *r, const BIGNUM *m, const BIGNUM *d, BN_CTX *ctx)
{
    /*
     * like BN_mod, but returns non-negative remainder (i.e., 0 <= r < |d|
     * always holds)
     */

    if (!(BN_mod(r, m, d, ctx)))
        return 0;
    if (!r->neg)
        return 1;
    /* now   -|d| < r < 0,  so we have to set  r := r + |d| */
    return (d->neg ? BN_sub : BN_add) (r, r, d);
}

int BN_mod_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m,
               BN_CTX *ctx)
{
    BIGNUM *t;
    int ret = 0;

    bn_check_top(a);
    bn_check_top(b);
    bn_check_top(m);

    BN_CTX_start(ctx);
    if ((t = BN_CTX_get(ctx)) == NULL)
        goto err;
    if (a == b) {
        if (!BN_sqr(t, a, ctx))
            goto err;
    } else {
        if (!BN_mul(t, a, b, ctx))
            goto err;
    }
    if (!BN_nnmod(r, t, m, ctx))
        goto err;
    bn_check_top(r);
    ret = 1;
 err:
    BN_CTX_end(ctx);
    return ret;
}


// crypto/bn/bn_word.c
int BN_add_word(BIGNUM *a, BN_ULONG w)
{
    BN_ULONG l;
    int i;

    bn_check_top(a);
    w &= BN_MASK2;

    /* degenerate case: w is zero */
    if (!w)
        return 1;
    /* degenerate case: a is zero */
    if (BN_is_zero(a))
        return BN_set_word(a, w);
    /* handle 'a' when negative */
    if (a->neg) {
        a->neg = 0;
        i = BN_sub_word(a, w);
        if (!BN_is_zero(a))
            a->neg = !(a->neg);
        return i;
    }
    for (i = 0; w != 0 && i < a->top; i++) {
        a->d[i] = l = (a->d[i] + w) & BN_MASK2;
        w = (w > l) ? 1 : 0;
    }
    if (w && i == a->top) {
        if (bn_wexpand(a, a->top + 1) == NULL)
            return 0;
        a->top++;
        a->d[i] = w;
    }
    bn_check_top(a);
    return 1;
}

int BN_sub_word(BIGNUM *a, BN_ULONG w)
{
    int i;

    bn_check_top(a);
    w &= BN_MASK2;

    /* degenerate case: w is zero */
    if (!w)
        return 1;
    /* degenerate case: a is zero */
    if (BN_is_zero(a)) {
        i = BN_set_word(a, w);
        if (i != 0)
            BN_set_negative(a, 1);
        return i;
    }
    /* handle 'a' when negative */
    if (a->neg) {
        a->neg = 0;
        i = BN_add_word(a, w);
        a->neg = 1;
        return i;
    }

    if ((a->top == 1) && (a->d[0] < w)) {
        a->d[0] = w - a->d[0];
        a->neg = 1;
        return 1;
    }
    i = 0;
    for (;;) {
        if (a->d[i] >= w) {
            a->d[i] -= w;
            break;
        } else {
            a->d[i] = (a->d[i] - w) & BN_MASK2;
            i++;
            w = 1;
        }
    }
    if ((a->d[i] == 0) && (i == (a->top - 1)))
        a->top--;
    bn_check_top(a);
    return 1;
}

int BN_mul_word(BIGNUM *a, BN_ULONG w)
{
    BN_ULONG ll;

    bn_check_top(a);
    w &= BN_MASK2;
    if (a->top) {
        if (w == 0)
            BN_zero(a);
        else {
            ll = bn_mul_words(a->d, a->d, a->top, w);
            if (ll) {
                if (bn_wexpand(a, a->top + 1) == NULL)
                    return 0;
                a->d[a->top++] = ll;
            }
        }
    }
    bn_check_top(a);
    return 1;
}

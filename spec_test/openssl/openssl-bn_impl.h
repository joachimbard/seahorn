#include <string.h>
#include <assert.h>
#include <limits.h>

#include "openssl-bn_ctx_impl.h"
#include "openssl-bn_lib_impl.h"

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

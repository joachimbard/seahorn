// include/openssl/types.h
typedef struct bio_st BIO;
typedef struct bignum_st BIGNUM;
typedef struct bignum_ctx BN_CTX;
typedef struct bn_blinding_st BN_BLINDING;
typedef struct bn_mont_ctx_st BN_MONT_CTX;
typedef struct bn_recp_ctx_st BN_RECP_CTX;
typedef struct bn_gencb_st BN_GENCB;
typedef struct buf_mem_st BUF_MEM;

// XXX: we keep the context abstract here
// definition in context.c
typedef struct ossl_lib_ctx_st OSSL_LIB_CTX;


// include/openssl/bn.h
// with SIXTY_FOUR_BIT_LONG
#define SIXTY_FOUR_BIT_LONG
# ifdef SIXTY_FOUR_BIT_LONG
#  define BN_ULONG        unsigned long
#  define BN_BYTES        8
# endif

# define BN_BITS2       (BN_BYTES * 8)
# define BN_BITS        (BN_BITS2 * 2)

# define BN_FLG_CONSTTIME        0x04
# define BN_FLG_SECURE           0x08
int BN_set_word(BIGNUM *a, BN_ULONG w);
#define BN_one(a) (BN_set_word((a),1))
void BN_zero_ex(BIGNUM *a);
#define BN_zero(a) BN_zero_ex(a)
int BN_num_bits_word(BN_ULONG l);
int BN_nnmod(BIGNUM *r, const BIGNUM *m, const BIGNUM *d, BN_CTX *ctx);
int BN_mod_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m,
               BN_CTX *ctx);
int BN_mod_add_quick(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                     const BIGNUM *m);
int BN_mod_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m,
               BN_CTX *ctx);
int BN_mod_sub_quick(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                     const BIGNUM *m);
int BN_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);
int BN_sqr(BIGNUM *r, const BIGNUM *a, BN_CTX *ctx);
int BN_mod_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m,
               BN_CTX *ctx);
int BN_mod_sqr(BIGNUM *r, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx);
int BN_mod_exp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
               const BIGNUM *m, BN_CTX *ctx);
int BN_mod_exp_mont(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
                    const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx);
int BN_mod_exp_mont_consttime(BIGNUM *rr, const BIGNUM *a, const BIGNUM *p,
                              const BIGNUM *m, BN_CTX *ctx,
                              BN_MONT_CTX *in_mont);
int BN_mod_exp_mont_word(BIGNUM *r, BN_ULONG a, const BIGNUM *p,
                         const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx);
int BN_mod_exp2_mont(BIGNUM *r, const BIGNUM *a1, const BIGNUM *p1,
                     const BIGNUM *a2, const BIGNUM *p2, const BIGNUM *m,
                     BN_CTX *ctx, BN_MONT_CTX *m_ctx);
int BN_mod_exp_simple(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
                      const BIGNUM *m, BN_CTX *ctx);
int BN_mod_exp_mont_consttime_x2(BIGNUM *rr1, const BIGNUM *a1, const BIGNUM *p1,
                                 const BIGNUM *m1, BN_MONT_CTX *in_mont1,
                                 BIGNUM *rr2, const BIGNUM *a2, const BIGNUM *p2,
                                 const BIGNUM *m2, BN_MONT_CTX *in_mont2,
                                 BN_CTX *ctx);

BN_RECP_CTX *BN_RECP_CTX_new(void);
void BN_RECP_CTX_free(BN_RECP_CTX *recp);
int BN_RECP_CTX_set(BN_RECP_CTX *recp, const BIGNUM *rdiv, BN_CTX *ctx);
int BN_mod_mul_reciprocal(BIGNUM *r, const BIGNUM *x, const BIGNUM *y,
                          BN_RECP_CTX *recp, BN_CTX *ctx);
int BN_mod_exp_recp(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
                    const BIGNUM *m, BN_CTX *ctx);
int BN_div_recp(BIGNUM *dv, BIGNUM *rem, const BIGNUM *m,
                BN_RECP_CTX *recp, BN_CTX *ctx);


// include/crypto/bn.h
int bn_mul_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);
int bn_sqr_fixed_top(BIGNUM *r, const BIGNUM *a, BN_CTX *ctx);


// bn_local.h
// with SIXTY_FOUR_BIT_LONG
#define BN_MASK2        (0xffffffffffffffffL)
// with !BN_DEBUG
#define bn_check_top(a)
#define BN_FLG_FIXED_TOP 0

BN_ULONG bn_mul_add_words(BN_ULONG *rp, const BN_ULONG *ap, int num,
                          BN_ULONG w);
BN_ULONG bn_mul_words(BN_ULONG *rp, const BN_ULONG *ap, int num, BN_ULONG w);
void bn_sqr_words(BN_ULONG *rp, const BN_ULONG *ap, int num);
BN_ULONG bn_div_words(BN_ULONG h, BN_ULONG l, BN_ULONG d);
BN_ULONG bn_add_words(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                      int num);
BN_ULONG bn_sub_words(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                      int num);

struct bignum_st {
    BN_ULONG *d;                /* Pointer to an array of 'BN_BITS2' bit
                                 * chunks. */
    int top;                    /* Index of last used d +1. */
    /* The next are internal book keeping for bn_expand. */
    int dmax;                   /* Size of the d array. */
    int neg;                    /* one if the number is negative */
    int flags;
};
/* Used for montgomery multiplication */
struct bn_mont_ctx_st {
    int ri;                     /* number of bits in R */
    BIGNUM RR;                  /* used to convert to montgomery form,
                                   possibly zero-padded */
    BIGNUM N;                   /* The modulus */
    BIGNUM Ni;                  /* R*(1/R mod N) - N*Ni = 1 (Ni is only
                                 * stored for bignum algorithm) */
    BN_ULONG n0[2];             /* least significant word(s) of Ni; (type
                                 * changed with 0.9.9, was "BN_ULONG n0;"
                                 * before) */
    int flags;
};

/*
 * Used for reciprocal division/mod functions It cannot be shared between
 * threads
 */
struct bn_recp_ctx_st {
    BIGNUM N;                   /* the divisor */
    BIGNUM Nr;                  /* the reciprocal */
    int num_bits;
    int shift;
    int flags;
};

# define BN_window_bits_for_exponent_size(b) \
                ((b) > 671 ? 6 : \
                 (b) > 239 ? 5 : \
                 (b) >  79 ? 4 : \
                 (b) >  23 ? 3 : 1)

# define BN_MULL_SIZE_NORMAL                     (16)/* 32 */
# define BN_MUL_RECURSIVE_SIZE_NORMAL            (16)/* 32 less than */
# define BN_SQR_RECURSIVE_SIZE_NORMAL            (16)/* 32 */
# define BN_MUL_LOW_RECURSIVE_SIZE_NORMAL        (32)/* 32 */
# define BN_MONT_CTX_SET_SIZE_WORD               (64)/* 32 */

void bn_init(BIGNUM *a);
void bn_mul_normal(BN_ULONG *r, BN_ULONG *a, int na, BN_ULONG *b, int nb);
void bn_mul_comba8(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b);
void bn_mul_comba4(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b);
void bn_sqr_normal(BN_ULONG *r, const BN_ULONG *a, int n, BN_ULONG *tmp);
void bn_sqr_comba8(BN_ULONG *r, const BN_ULONG *a);
void bn_sqr_comba4(BN_ULONG *r, const BN_ULONG *a);
int bn_cmp_words(const BN_ULONG *a, const BN_ULONG *b, int n);
int bn_cmp_part_words(const BN_ULONG *a, const BN_ULONG *b, int cl, int dl);
void bn_mul_recursive(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b, int n2,
                      int dna, int dnb, BN_ULONG *t);
void bn_mul_part_recursive(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b,
                           int n, int tna, int tnb, BN_ULONG *t);
void bn_sqr_recursive(BN_ULONG *r, const BN_ULONG *a, int n2, BN_ULONG *t);
void bn_mul_low_normal(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b, int n);
void bn_mul_low_recursive(BN_ULONG *r, BN_ULONG *a, BN_ULONG *b, int n2,
                          BN_ULONG *t);
BN_ULONG bn_sub_part_words(BN_ULONG *r, const BN_ULONG *a, const BN_ULONG *b,
                           int cl, int dl);
int bn_mul_mont(BN_ULONG *rp, const BN_ULONG *ap, const BN_ULONG *bp,
                const BN_ULONG *np, const BN_ULONG *n0, int num);


// bn_ctx.c
/* How many bignums are in each "pool item"; */
#define BN_CTX_POOL_SIZE 16
/* A bundle of bignums that can be linked with other bundles */
typedef struct bignum_pool_item {
    /* The bignum values */
    BIGNUM vals[BN_CTX_POOL_SIZE];
    /* Linked-list admin */
    struct bignum_pool_item *prev, *next;
} BN_POOL_ITEM;
/* A linked-list of bignums grouped in bundles */
typedef struct bignum_pool {
    /* Linked-list admin */
    BN_POOL_ITEM *head, *current, *tail;
    /* Stack depth and allocation size */
    unsigned used, size;
} BN_POOL;

/* A wrapper to manage the "stack frames" */
typedef struct bignum_ctx_stack {
    /* Array of indexes into the bignum stack */
    unsigned int *indexes;
    /* Number of stack frames, and the size of the allocated array */
    unsigned int depth, size;
} BN_STACK;

/* The opaque BN_CTX type */
struct bignum_ctx {
    /* The bignum bundles */
    BN_POOL pool;
    /* The "stack frames", if you will */
    BN_STACK stack;
    /* The number of bignums currently assigned */
    unsigned int used;
    /* Depth of stack overflow */
    int err_stack;
    /* Block "gets" until an "end" (compatibility behaviour) */
    int too_many;
    /* Flags. */
    int flags;
    /* The library context */
    OSSL_LIB_CTX *libctx;
};

BN_CTX *BN_CTX_new(void);

// defintions are in bn_ctx.c
void BN_CTX_start(BN_CTX *ctx);
BIGNUM *BN_CTX_get(BN_CTX *ctx);
void BN_CTX_end(BN_CTX *ctx);

// defintions are in bn_lib.c
BIGNUM *BN_copy(BIGNUM *a, const BIGNUM *b);
int BN_num_bits(const BIGNUM *a);
int BN_abs_is_word(const BIGNUM *a, const BN_ULONG w);
int BN_is_zero(const BIGNUM *a);
int BN_is_one(const BIGNUM *a);
int BN_is_word(const BIGNUM *a, const BN_ULONG w);
int BN_is_odd(const BIGNUM *a);
int BN_is_bit_set(const BIGNUM *a, int n);
void bn_correct_top(BIGNUM *a);
BIGNUM *bn_wexpand(BIGNUM *a, int words);


// bn_lib.c
//static BN_ULONG *bn_expand_internal(const BIGNUM *b, int words)
//{
//    BN_ULONG *a = NULL;
//
//    if (words > (INT_MAX / (4 * BN_BITS2))) {
//        ERR_raise(ERR_LIB_BN, BN_R_BIGNUM_TOO_LONG);
//        return NULL;
//    }
//    if (BN_get_flags(b, BN_FLG_STATIC_DATA)) {
//        ERR_raise(ERR_LIB_BN, BN_R_EXPAND_ON_STATIC_BIGNUM_DATA);
//        return NULL;
//    }
//    if (BN_get_flags(b, BN_FLG_SECURE))
//        a = OPENSSL_secure_zalloc(words * sizeof(*a));
//    else
//        a = OPENSSL_zalloc(words * sizeof(*a));
//    if (a == NULL) {
//        ERR_raise(ERR_LIB_BN, ERR_R_MALLOC_FAILURE);
//        return NULL;
//    }
//
//    assert(b->top <= words);
//    if (b->top > 0)
//        memcpy(a, b->d, sizeof(*a) * b->top);
//
//    return a;
//}
//BIGNUM *bn_expand2(BIGNUM *b, int words)
//{
//    if (words > b->dmax) {
//        BN_ULONG *a = bn_expand_internal(b, words);
//        if (!a)
//            return NULL;
//        if (b->d != NULL)
//            bn_free_d(b, 1);
//        b->d = a;
//        b->dmax = words;
//    }
//
//    return b;
//}
//int BN_is_bit_set(const BIGNUM *a, int n)
//{
//    int i, j;
//
//    bn_check_top(a);
//    if (n < 0)
//        return 0;
//    i = n / BN_BITS2;
//    j = n % BN_BITS2;
//    if (a->top <= i)
//        return 0;
//    return (int)(((a->d[i]) >> j) & ((BN_ULONG)1));
//}
//int BN_is_odd(const BIGNUM *a)
//{
//    return (a->top > 0) && (a->d[0] & 1);
//}
int BN_get_flags(const BIGNUM *b, int n)
{
    return b->flags & n;
}
//BIGNUM *bn_wexpand(BIGNUM *a, int words)
//{
//    return (words <= a->dmax) ? a : bn_expand2(a, words);
//}
//void bn_correct_top(BIGNUM *a)
//{
//    BN_ULONG *ftl;
//    int tmp_top = a->top;
//
//    if (tmp_top > 0) {
//        for (ftl = &(a->d[tmp_top]); tmp_top > 0; tmp_top--) {
//            ftl--;
//            if (*ftl != 0)
//                break;
//        }
//        a->top = tmp_top;
//    }
//    if (a->top == 0)
//        a->neg = 0;
//    a->flags &= ~BN_FLG_FIXED_TOP;
//    bn_pollute(a);
//}


// include/openssl/macros.h
# ifndef OPENSSL_FILE
#  ifdef OPENSSL_NO_FILENAMES
#   define OPENSSL_FILE ""
#   define OPENSSL_LINE 0
#  else
#   define OPENSSL_FILE __FILE__
#   define OPENSSL_LINE __LINE__
#  endif
# endif

#    define OPENSSL_FUNC __func__


// include/openssl/err.h.in
# define ERR_LIB_OFFSET                 23L
# define ERR_LIB_MASK                   0xFF
# define ERR_RFLAGS_OFFSET              18L
# define ERR_RFLAGS_MASK                0x1F
# define ERR_REASON_MASK                0X7FFFFF

# define ERR_RFLAG_FATAL                (0x1 << ERR_RFLAGS_OFFSET)
# define ERR_RFLAG_COMMON               (0x2 << ERR_RFLAGS_OFFSET)

# define ERR_LIB_BN              3
# define ERR_R_FATAL                             (ERR_RFLAG_FATAL|ERR_RFLAG_COMMON)
# define ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED       (257|ERR_R_FATAL)
void ERR_new(void);
void ERR_set_debug(const char *file, int line, const char *func);
void ERR_set_error(int lib, int reason, const char *fmt, ...);
# define ERR_raise(lib, reason) ERR_raise_data((lib),(reason),NULL)
# define ERR_raise_data                                         \
    (ERR_new(),                                                 \
     ERR_set_debug(OPENSSL_FILE,OPENSSL_LINE,OPENSSL_FUNC),     \
     ERR_set_error)

#include <stdlib.h>

// include/openssl/e_os2.h
#define ossl_inline inline

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


// include/openssl/crypto.h.in
typedef void CRYPTO_RWLOCK;
# define OPENSSL_free(addr) \
        CRYPTO_free(addr, OPENSSL_FILE, OPENSSL_LINE)

# define OPENSSL_malloc(num) \
        CRYPTO_malloc(num, OPENSSL_FILE, OPENSSL_LINE)
# define OPENSSL_zalloc(num) \
        CRYPTO_zalloc(num, OPENSSL_FILE, OPENSSL_LINE)
# define OPENSSL_secure_zalloc(num) \
        CRYPTO_secure_zalloc(num, OPENSSL_FILE, OPENSSL_LINE)
typedef void *(*CRYPTO_malloc_fn)(size_t num, const char *file, int line);
typedef void (*CRYPTO_free_fn)(void *addr, const char *file, int line);
void *CRYPTO_malloc(size_t num, const char *file, int line);
void *CRYPTO_zalloc(size_t num, const char *file, int line);
void CRYPTO_free(void *ptr, const char *file, int line);
void *CRYPTO_secure_zalloc(size_t num, const char *file, int line);
void OPENSSL_cleanse(void *ptr, size_t len);


// include/openssl/bn.h
BIGNUM *bn_wexpand(BIGNUM *a, int words);
BIGNUM *bn_expand2(BIGNUM *a, int words);
void bn_correct_top(BIGNUM *a);

// with SIXTY_FOUR_BIT_LONG
#define SIXTY_FOUR_BIT_LONG
# ifdef SIXTY_FOUR_BIT_LONG
#  define BN_ULONG        unsigned long
#  define BN_BYTES        8
# endif

# define BN_BITS2       (BN_BYTES * 8)
# define BN_BITS        (BN_BITS2 * 2)

# define BN_FLG_MALLOCED         0x01
# define BN_FLG_STATIC_DATA      0x02

# define BN_FLG_CONSTTIME        0x04
# define BN_FLG_SECURE           0x08

void BN_set_flags(BIGNUM *b, int n);
int BN_get_flags(const BIGNUM *b, int n);

int BN_abs_is_word(const BIGNUM *a, const BN_ULONG w);
int BN_is_zero(const BIGNUM *a);
int BN_is_one(const BIGNUM *a);
int BN_is_word(const BIGNUM *a, const BN_ULONG w);
int BN_is_odd(const BIGNUM *a);

int BN_ucmp(const BIGNUM *a, const BIGNUM *b);
int BN_set_bit(BIGNUM *a, int n);
int BN_clear_bit(BIGNUM *a, int n);

BN_ULONG BN_mod_word(const BIGNUM *a, BN_ULONG w);
BN_ULONG BN_div_word(BIGNUM *a, BN_ULONG w);
int BN_mul_word(BIGNUM *a, BN_ULONG w);
int BN_add_word(BIGNUM *a, BN_ULONG w);
int BN_sub_word(BIGNUM *a, BN_ULONG w);
int BN_set_word(BIGNUM *a, BN_ULONG w);
BN_ULONG BN_get_word(const BIGNUM *a);

#define BN_one(a) (BN_set_word((a),1))
void BN_zero_ex(BIGNUM *a);
#define BN_zero(a) BN_zero_ex(a)

const BIGNUM *BN_value_one(void);
char *BN_options(void);
BN_CTX *BN_CTX_new_ex(OSSL_LIB_CTX *ctx);
BN_CTX *BN_CTX_new(void);
BN_CTX *BN_CTX_secure_new_ex(OSSL_LIB_CTX *ctx);
BN_CTX *BN_CTX_secure_new(void);
void BN_CTX_free(BN_CTX *c);
void BN_CTX_start(BN_CTX *ctx);
BIGNUM *BN_CTX_get(BN_CTX *ctx);
void BN_CTX_end(BN_CTX *ctx);
int BN_num_bits(const BIGNUM *a);
int BN_num_bits_word(BN_ULONG l);
int BN_security_bits(int L, int N);
BIGNUM *BN_new(void);
BIGNUM *BN_secure_new(void);
void BN_clear_free(BIGNUM *a);
BIGNUM *BN_copy(BIGNUM *a, const BIGNUM *b);
void BN_swap(BIGNUM *a, BIGNUM *b);

int BN_cmp(const BIGNUM *a, const BIGNUM *b);
void BN_free(BIGNUM *a);
int BN_is_bit_set(const BIGNUM *a, int n);

int BN_div(BIGNUM *dv, BIGNUM *rem, const BIGNUM *m, const BIGNUM *d,
           BN_CTX *ctx);
# define BN_mod(rem,m,d,ctx) BN_div(NULL,(rem),(m),(d),(ctx))
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

BN_MONT_CTX *BN_MONT_CTX_new(void);
int BN_mod_mul_montgomery(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                          BN_MONT_CTX *mont, BN_CTX *ctx);
int BN_to_montgomery(BIGNUM *r, const BIGNUM *a, BN_MONT_CTX *mont,
                     BN_CTX *ctx);
int BN_from_montgomery(BIGNUM *r, const BIGNUM *a, BN_MONT_CTX *mont,
                       BN_CTX *ctx);
void BN_MONT_CTX_free(BN_MONT_CTX *mont);
int BN_MONT_CTX_set(BN_MONT_CTX *mont, const BIGNUM *mod, BN_CTX *ctx);
BN_MONT_CTX *BN_MONT_CTX_copy(BN_MONT_CTX *to, BN_MONT_CTX *from);
BN_MONT_CTX *BN_MONT_CTX_set_locked(BN_MONT_CTX **pmont, CRYPTO_RWLOCK *lock,
                                    const BIGNUM *mod, BN_CTX *ctx);

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
int bn_mul_mont_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                          BN_MONT_CTX *mont, BN_CTX *ctx);
int bn_to_mont_fixed_top(BIGNUM *r, const BIGNUM *a, BN_MONT_CTX *mont,
                         BN_CTX *ctx);
int bn_from_mont_fixed_top(BIGNUM *r, const BIGNUM *a, BN_MONT_CTX *mont,
                           BN_CTX *ctx);
int bn_mod_add_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                         const BIGNUM *m);
int bn_mod_sub_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
                         const BIGNUM *m);
int bn_mul_fixed_top(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx);
int bn_sqr_fixed_top(BIGNUM *r, const BIGNUM *a, BN_CTX *ctx);


// bn_local.h
// XXX: with SIXTY_FOUR_BIT_LONG
#define BN_MASK2        (0xffffffffffffffffL)
// XXX: with !BN_DEBUG
#define BN_FLG_FIXED_TOP 0
#define bn_pollute(a)
#define bn_check_top(a)
#define bn_fix_top(a)           bn_correct_top(a)
#define bn_check_size(bn, bits)
#define bn_wcheck_size(bn, words)

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

# define MOD_EXP_CTIME_MIN_CACHE_LINE_WIDTH      ( 64 )
# define MOD_EXP_CTIME_MIN_CACHE_LINE_MASK       (MOD_EXP_CTIME_MIN_CACHE_LINE_WIDTH - 1)

/*
 * Window sizes optimized for fixed window size modular exponentiation
 * algorithm (BN_mod_exp_mont_consttime). To achieve the security goals of
 * BN_mode_exp_mont_consttime, the maximum size of the window must not exceed
 * log_2(MOD_EXP_CTIME_MIN_CACHE_LINE_WIDTH). Window size thresholds are
 * defined for cache line sizes of 32 and 64, cache line sizes where
 * log_2(32)=5 and log_2(64)=6 respectively. A window size of 7 should only be
 * used on processors that have a 128 byte or greater cache line size.
 */
# if MOD_EXP_CTIME_MIN_CACHE_LINE_WIDTH == 64

#  define BN_window_bits_for_ctime_exponent_size(b) \
                ((b) > 937 ? 6 : \
                 (b) > 306 ? 5 : \
                 (b) >  89 ? 4 : \
                 (b) >  22 ? 3 : 1)
#  define BN_MAX_WINDOW_BITS_FOR_CTIME_EXPONENT_SIZE    (6)
# endif

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


// defintions are in bn_lib.c
static void bn_free_d(BIGNUM *a, int clear);


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
# define ERR_R_MALLOC_FAILURE                    (256|ERR_R_FATAL)
# define ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED       (257|ERR_R_FATAL)
# define ERR_R_PASSED_NULL_PARAMETER             (258|ERR_R_FATAL)
# define ERR_R_INTERNAL_ERROR                    (259|ERR_R_FATAL)
# define ERR_R_DISABLED                          (260|ERR_R_FATAL)
# define ERR_R_INIT_FAIL                         (261|ERR_R_FATAL)
# define ERR_R_PASSED_INVALID_ARGUMENT           (262|ERR_RFLAG_COMMON)
# define ERR_R_OPERATION_FAIL                    (263|ERR_R_FATAL)
# define ERR_R_INVALID_PROVIDER_FUNCTIONS        (264|ERR_R_FATAL)
# define ERR_R_INTERRUPTED_OR_CANCELLED          (265|ERR_RFLAG_COMMON)
# define ERR_R_NESTED_ASN1_ERROR                 (266|ERR_RFLAG_COMMON)
# define ERR_R_MISSING_ASN1_EOS                  (267|ERR_RFLAG_COMMON)
# define ERR_R_UNSUPPORTED                       (268|ERR_RFLAG_COMMON)
# define ERR_R_FETCH_FAILED                      (269|ERR_RFLAG_COMMON)
# define ERR_R_INVALID_PROPERTY_DEFINITION       (270|ERR_RFLAG_COMMON)
# define ERR_R_UNABLE_TO_GET_READ_LOCK           (271|ERR_R_FATAL)
# define ERR_R_UNABLE_TO_GET_WRITE_LOCK          (272|ERR_R_FATAL)
void ERR_new(void);
void ERR_set_debug(const char *file, int line, const char *func);
void ERR_set_error(int lib, int reason, const char *fmt, ...);
# define ERR_raise(lib, reason) ERR_raise_data((lib),(reason),NULL)
# define ERR_raise_data                                         \
    (ERR_new(),                                                 \
     ERR_set_debug(OPENSSL_FILE,OPENSSL_LINE,OPENSSL_FUNC),     \
     ERR_set_error)


// include/openssl/bnerr.h
# define BN_R_ARG2_LT_ARG3                                100
# define BN_R_BAD_RECIPROCAL                              101
# define BN_R_BIGNUM_TOO_LONG                             114
# define BN_R_BITS_TOO_SMALL                              118
# define BN_R_CALLED_WITH_EVEN_MODULUS                    102
# define BN_R_DIV_BY_ZERO                                 103
# define BN_R_ENCODING_ERROR                              104
# define BN_R_EXPAND_ON_STATIC_BIGNUM_DATA                105
# define BN_R_INPUT_NOT_REDUCED                           110
# define BN_R_INVALID_LENGTH                              106
# define BN_R_INVALID_RANGE                               115
# define BN_R_INVALID_SHIFT                               119
# define BN_R_NOT_A_SQUARE                                111
# define BN_R_NOT_INITIALIZED                             107
# define BN_R_NO_INVERSE                                  108
# define BN_R_NO_SOLUTION                                 116
# define BN_R_NO_SUITABLE_DIGEST                          120
# define BN_R_PRIVATE_KEY_TOO_LARGE                       117
# define BN_R_P_IS_NOT_PRIME                              112
# define BN_R_TOO_MANY_ITERATIONS                         113
# define BN_R_TOO_MANY_TEMPORARY_VARIABLES                109


// include/internal/constant_time.h
static ossl_inline unsigned int constant_time_eq_int(int a, int b);


// include/openssl/trace.h
// XXX: with OPENSSL_NO_TRACE
#define OSSL_TRACE_BEGIN(category)           \
  do {                                        \
      BIO *trc_out = NULL;                    \
      if (0)

#define OSSL_TRACE_END(category)             \
  } while(0)

#define OSSL_TRACE_CANCEL(category)          \
    ((void)0)


// include/openssl/bio.h.in
int BIO_printf(BIO *bio, const char *format, ...);

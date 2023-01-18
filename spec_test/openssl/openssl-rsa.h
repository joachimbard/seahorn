#include <stdint.h>
#include "openssl-bn.h"

#define FIPS_MODULE
#define OPENSSL_NO_ACVP_TESTS
#define OPENSSL_SUPPRESS_DEPRECATED

#include "openssl-rsaerr.h"


// include/openssl/err.h.in
# define ERR_LIB_NONE            1
# define ERR_LIB_SYS             2
# define ERR_LIB_BN              3
# define ERR_LIB_RSA             4
# define ERR_LIB_DH              5
# define ERR_LIB_EVP             6
# define ERR_LIB_BUF             7
# define ERR_LIB_OBJ             8
# define ERR_LIB_PEM             9
# define ERR_LIB_DSA             10
# define ERR_LIB_X509            11


// include/internal/refcount.h
typedef _Atomic int CRYPTO_REF_COUNT;


// include/openssl/crypto.h.in
typedef void CRYPTO_RWLOCK;


// include/openssl/types.h
typedef struct rsa_st RSA;
typedef struct rsa_meth_st RSA_METHOD;
typedef struct rsa_pss_params_st RSA_PSS_PARAMS;
typedef struct engine_st ENGINE;

typedef struct evp_md_st EVP_MD;

typedef struct evp_pkey_ctx_st EVP_PKEY_CTX;


// include/openssl/macros.h
# ifndef OSSL_DEPRECATED
#  define OSSL_DEPRECATED(since)                extern
#  define OSSL_DEPRECATED_FOR(since, message)   extern
# endif

#define OSSL_DEPRECATEDIN_3_0                OSSL_DEPRECATED(3.0)

//#include "openssl-rsa.h"
// include/openssl/rsa.h
#  define RSA_ASN1_VERSION_DEFAULT        0
#  define RSA_ASN1_VERSION_MULTI          1

#  define RSA_METHOD_FLAG_NO_CHECK        0x0001
#  define RSA_FLAG_CACHE_PUBLIC           0x0002
#  define RSA_FLAG_CACHE_PRIVATE          0x0004
#  define RSA_FLAG_BLINDING               0x0008
#  define RSA_FLAG_THREAD_SAFE            0x0010

#  define RSA_FLAG_EXT_PKEY               0x0020

#  define RSA_FLAG_NO_BLINDING            0x0080

# define RSA_PKCS1_PADDING          1
# define RSA_NO_PADDING             3
# define RSA_PKCS1_OAEP_PADDING     4
# define RSA_X931_PADDING           5

OSSL_DEPRECATEDIN_3_0 int RSA_blinding_on(RSA *rsa, BN_CTX *ctx);
OSSL_DEPRECATEDIN_3_0 void RSA_blinding_off(RSA *rsa);
OSSL_DEPRECATEDIN_3_0 BN_BLINDING *RSA_setup_blinding(RSA *rsa, BN_CTX *ctx);

OSSL_DEPRECATEDIN_3_0
int RSA_padding_add_PKCS1_type_1(unsigned char *to, int tlen,
                                 const unsigned char *f, int fl);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_check_PKCS1_type_1(unsigned char *to, int tlen,
                                   const unsigned char *f, int fl,
                                   int rsa_len);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_add_PKCS1_type_2(unsigned char *to, int tlen,
                                 const unsigned char *f, int fl);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_check_PKCS1_type_2(unsigned char *to, int tlen,
                                   const unsigned char *f, int fl,
                                   int rsa_len);
OSSL_DEPRECATEDIN_3_0 int PKCS1_MGF1(unsigned char *mask, long len,
                                     const unsigned char *seed, long seedlen,
                                     const EVP_MD *dgst);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_add_PKCS1_OAEP(unsigned char *to, int tlen,
                               const unsigned char *f, int fl,
                               const unsigned char *p, int pl);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_check_PKCS1_OAEP(unsigned char *to, int tlen,
                                 const unsigned char *f, int fl, int rsa_len,
                                 const unsigned char *p, int pl);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_add_PKCS1_OAEP_mgf1(unsigned char *to, int tlen,
                                    const unsigned char *from, int flen,
                                    const unsigned char *param, int plen,
                                    const EVP_MD *md, const EVP_MD *mgf1md);
OSSL_DEPRECATEDIN_3_0
int RSA_padding_check_PKCS1_OAEP_mgf1(unsigned char *to, int tlen,
                                      const unsigned char *from, int flen,
                                      int num,
                                      const unsigned char *param, int plen,
                                      const EVP_MD *md, const EVP_MD *mgf1md);
OSSL_DEPRECATEDIN_3_0 int RSA_padding_add_none(unsigned char *to, int tlen,
                                               const unsigned char *f, int fl);
OSSL_DEPRECATEDIN_3_0 int RSA_padding_check_none(unsigned char *to, int tlen,
                                                 const unsigned char *f, int fl,
                                                 int rsa_len);
OSSL_DEPRECATEDIN_3_0 int RSA_padding_add_X931(unsigned char *to, int tlen,
                                               const unsigned char *f, int fl);
OSSL_DEPRECATEDIN_3_0 int RSA_padding_check_X931(unsigned char *to, int tlen,
                                                 const unsigned char *f, int fl,
                                                 int rsa_len);
OSSL_DEPRECATEDIN_3_0 int RSA_X931_hash_id(int nid);

// include/crypto/rsa.h
#define RSA_MIN_MODULUS_BITS    512

typedef struct rsa_pss_params_30_st {
    int hash_algorithm_nid;
    struct {
        int algorithm_nid;       /* Currently always NID_mgf1 */
        int hash_algorithm_nid;
    } mask_gen;
    int salt_len;
    int trailer_field;
} RSA_PSS_PARAMS_30;


// rsa_local.h
struct rsa_st {
    /*
     * #legacy
     * The first field is used to pickup errors where this is passed
     * instead of an EVP_PKEY.  It is always zero.
     * THIS MUST REMAIN THE FIRST FIELD.
     */
    int dummy_zero;

    OSSL_LIB_CTX *libctx;
    int32_t version;
    const RSA_METHOD *meth;
    /* functional reference if 'meth' is ENGINE-provided */
    ENGINE *engine;
    BIGNUM *n;
    BIGNUM *e;
    BIGNUM *d;
    BIGNUM *p;
    BIGNUM *q;
    BIGNUM *dmp1;
    BIGNUM *dmq1;
    BIGNUM *iqmp;

    /*
     * If a PSS only key this contains the parameter restrictions.
     * There are two structures for the same thing, used in different cases.
     */
    /* This is used uniquely by OpenSSL provider implementations. */
    RSA_PSS_PARAMS_30 pss_params;

#if defined(FIPS_MODULE) && !defined(OPENSSL_NO_ACVP_TESTS)
    RSA_ACVP_TEST *acvp_test;
#endif

#ifndef FIPS_MODULE
    /* This is used uniquely by rsa_ameth.c and rsa_pmeth.c. */
    RSA_PSS_PARAMS *pss;
    /* for multi-prime RSA, defined in RFC 8017 */
    STACK_OF(RSA_PRIME_INFO) *prime_infos;
    /* Be careful using this if the RSA structure is shared */
    CRYPTO_EX_DATA ex_data;
#endif
    CRYPTO_REF_COUNT references;
    int flags;
    /* Used to cache montgomery values */
    BN_MONT_CTX *_method_mod_n;
    BN_MONT_CTX *_method_mod_p;
    BN_MONT_CTX *_method_mod_q;
    BN_BLINDING *blinding;
    BN_BLINDING *mt_blinding;
    CRYPTO_RWLOCK *lock;

    int dirty_cnt;
};

struct rsa_meth_st {
    char *name;
    int (*rsa_pub_enc) (int flen, const unsigned char *from,
                        unsigned char *to, RSA *rsa, int padding);
    int (*rsa_pub_dec) (int flen, const unsigned char *from,
                        unsigned char *to, RSA *rsa, int padding);
    int (*rsa_priv_enc) (int flen, const unsigned char *from,
                         unsigned char *to, RSA *rsa, int padding);
    int (*rsa_priv_dec) (int flen, const unsigned char *from,
                         unsigned char *to, RSA *rsa, int padding);
    /* Can be null */
    int (*rsa_mod_exp) (BIGNUM *r0, const BIGNUM *I, RSA *rsa, BN_CTX *ctx);
    /* Can be null */
    int (*bn_mod_exp) (BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
                       const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx);
    /* called at new */
    int (*init) (RSA *rsa);
    /* called at free */
    int (*finish) (RSA *rsa);
    /* RSA_METHOD_FLAG_* things */
    int flags;
    /* may be needed! */
    char *app_data;
    /*
     * New sign and verify functions: some libraries don't allow arbitrary
     * data to be signed/verified: this allows them to be used. Note: for
     * this to work the RSA_public_decrypt() and RSA_private_encrypt() should
     * *NOT* be used. RSA_sign(), RSA_verify() should be used instead.
     */
    int (*rsa_sign) (int type,
                     const unsigned char *m, unsigned int m_length,
                     unsigned char *sigret, unsigned int *siglen,
                     const RSA *rsa);
    int (*rsa_verify) (int dtype, const unsigned char *m,
                       unsigned int m_length, const unsigned char *sigbuf,
                       unsigned int siglen, const RSA *rsa);
    /*
     * If this callback is NULL, the builtin software RSA key-gen will be
     * used. This is for behavioural compatibility whilst the code gets
     * rewired, but one day it would be nice to assume there are no such
     * things as "builtin software" implementations.
     */
    int (*rsa_keygen) (RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);
    int (*rsa_multi_prime_keygen) (RSA *rsa, int bits, int primes,
                                   BIGNUM *e, BN_GENCB *cb);
};

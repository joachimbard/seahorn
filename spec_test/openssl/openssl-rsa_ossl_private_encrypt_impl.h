static BN_BLINDING *rsa_get_blinding(RSA *rsa, int *local, BN_CTX *ctx)
{
    BN_BLINDING *ret;

    if (!CRYPTO_THREAD_write_lock(rsa->lock))
        return NULL;

    if (rsa->blinding == NULL) {
        rsa->blinding = RSA_setup_blinding(rsa, ctx);
    }

    ret = rsa->blinding;
    if (ret == NULL)
        goto err;

    if (BN_BLINDING_is_current_thread(ret)) {
        /* rsa->blinding is ours! */

        *local = 1;
    } else {
        /* resort to rsa->mt_blinding instead */

        /*
         * instructs rsa_blinding_convert(), rsa_blinding_invert() that the
         * BN_BLINDING is shared, meaning that accesses require locks, and
         * that the blinding factor must be stored outside the BN_BLINDING
         */
        *local = 0;

        if (rsa->mt_blinding == NULL) {
            rsa->mt_blinding = RSA_setup_blinding(rsa, ctx);
        }
        ret = rsa->mt_blinding;
    }

 err:
    CRYPTO_THREAD_unlock(rsa->lock);
    return ret;
}

static int rsa_blinding_convert(BN_BLINDING *b, BIGNUM *f, BIGNUM *unblind,
                                BN_CTX *ctx)
{
    if (unblind == NULL) {
        /*
         * Local blinding: store the unblinding factor in BN_BLINDING.
         */
        return BN_BLINDING_convert_ex(f, NULL, b, ctx);
    } else {
        /*
         * Shared blinding: store the unblinding factor outside BN_BLINDING.
         */
        int ret;

        BN_BLINDING_lock(b);
        ret = BN_BLINDING_convert_ex(f, unblind, b, ctx);
        BN_BLINDING_unlock(b);

        return ret;
    }
}

static int rsa_blinding_invert(BN_BLINDING *b, BIGNUM *f, BIGNUM *unblind,
                               BN_CTX *ctx)
{
    /*
     * For local blinding, unblind is set to NULL, and BN_BLINDING_invert_ex
     * will use the unblinding factor stored in BN_BLINDING. If BN_BLINDING
     * is shared between threads, unblind must be non-null:
     * BN_BLINDING_invert_ex will then use the local unblinding factor, and
     * will only read the modulus from BN_BLINDING. In both cases it's safe
     * to access the blinding without a lock.
     */
    return BN_BLINDING_invert_ex(f, unblind, b, ctx);
}

static int rsa_ossl_private_encrypt(int flen, const unsigned char *from,
                                   unsigned char *to, RSA *rsa, int padding)
{
    BIGNUM *f, *ret, *res;
    int i, num = 0, r = -1;
    unsigned char *buf = NULL;
    BN_CTX *ctx = NULL;
    int local_blinding = 0;
    /*
     * Used only if the blinding structure is shared. A non-NULL unblind
     * instructs rsa_blinding_convert() and rsa_blinding_invert() to store
     * the unblinding factor outside the blinding structure.
     */
    BIGNUM *unblind = NULL;
    BN_BLINDING *blinding = NULL;

    if ((ctx = BN_CTX_new_ex(rsa->libctx)) == NULL)
        goto err;
    BN_CTX_start(ctx);
    f = BN_CTX_get(ctx);
    ret = BN_CTX_get(ctx);
    num = BN_num_bytes(rsa->n);
    buf = OPENSSL_malloc(num);
    if (ret == NULL || buf == NULL) {
        ERR_raise(ERR_LIB_RSA, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    switch (padding) {
    case RSA_PKCS1_PADDING:
        i = RSA_padding_add_PKCS1_type_1(buf, num, from, flen);
        break;
    case RSA_X931_PADDING:
        i = RSA_padding_add_X931(buf, num, from, flen);
        break;
    case RSA_NO_PADDING:
        i = RSA_padding_add_none(buf, num, from, flen);
        break;
    default:
        ERR_raise(ERR_LIB_RSA, RSA_R_UNKNOWN_PADDING_TYPE);
        goto err;
    }
    if (i <= 0)
        goto err;

    if (BN_bin2bn(buf, num, f) == NULL)
        goto err;

    if (BN_ucmp(f, rsa->n) >= 0) {
        /* usually the padding functions would catch this */
        ERR_raise(ERR_LIB_RSA, RSA_R_DATA_TOO_LARGE_FOR_MODULUS);
        goto err;
    }

    if (rsa->flags & RSA_FLAG_CACHE_PUBLIC)
        if (!BN_MONT_CTX_set_locked(&rsa->_method_mod_n, rsa->lock,
                                    rsa->n, ctx))
            goto err;

    if (!(rsa->flags & RSA_FLAG_NO_BLINDING)) {
        blinding = rsa_get_blinding(rsa, &local_blinding, ctx);
        if (blinding == NULL) {
            ERR_raise(ERR_LIB_RSA, ERR_R_INTERNAL_ERROR);
            goto err;
        }
    }

    if (blinding != NULL) {
        if (!local_blinding && ((unblind = BN_CTX_get(ctx)) == NULL)) {
            ERR_raise(ERR_LIB_RSA, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        if (!rsa_blinding_convert(blinding, f, unblind, ctx))
            goto err;
    }

    if ((rsa->flags & RSA_FLAG_EXT_PKEY) ||
        (rsa->version == RSA_ASN1_VERSION_MULTI) ||
        ((rsa->p != NULL) &&
         (rsa->q != NULL) &&
         (rsa->dmp1 != NULL) && (rsa->dmq1 != NULL) && (rsa->iqmp != NULL))) {
        if (!rsa->meth->rsa_mod_exp(ret, f, rsa, ctx))
            goto err;
    } else {
        BIGNUM *d = BN_new();
        if (d == NULL) {
            ERR_raise(ERR_LIB_RSA, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        if (rsa->d == NULL) {
            ERR_raise(ERR_LIB_RSA, RSA_R_MISSING_PRIVATE_KEY);
            BN_free(d);
            goto err;
        }
        BN_with_flags(d, rsa->d, BN_FLG_CONSTTIME);

        if (!rsa->meth->bn_mod_exp(ret, f, d, rsa->n, ctx,
                                   rsa->_method_mod_n)) {
            BN_free(d);
            goto err;
        }
        /* We MUST free d before any further use of rsa->d */
        BN_free(d);
    }

    if (blinding)
        if (!rsa_blinding_invert(blinding, ret, unblind, ctx))
            goto err;

    if (padding == RSA_X931_PADDING) {
        if (!BN_sub(f, rsa->n, ret))
            goto err;
        if (BN_cmp(ret, f) > 0)
            res = f;
        else
            res = ret;
    } else {
        res = ret;
    }

    /*
     * BN_bn2binpad puts in leading 0 bytes if the number is less than
     * the length of the modulus.
     */
    r = BN_bn2binpad(res, to, num);
 err:
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    OPENSSL_clear_free(buf, num);
    return r;
}

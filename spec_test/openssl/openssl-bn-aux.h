BIGNUM* init_bn(BIGNUM*);
extern BN_ULONG *nd_ulong();
extern int nd_int();

void display_ctx(BN_CTX*);
void display_bn(BIGNUM*);
void display_ulong(BN_ULONG*);
void display_int(int);

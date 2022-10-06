BN_CTX* init_ctx();
BIGNUM* init_bn(BN_CTX*);
void nd_ulong(BN_ULONG*);
int nd_int();

void start_clock();
void end_clock();

void display_ctx(BN_CTX*);
void display_bn(BIGNUM*);
void display_ulong(BN_ULONG*);
void display_int(int);

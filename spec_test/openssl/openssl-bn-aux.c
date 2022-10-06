#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "openssl-bn.h"
#include "openssl-bn-aux.h"

unsigned seed;
double tstart = 0.0;

void init_seed(char** arg) {
  seed = strtoul(arg[1], NULL, 16);
  printf("seed: 0x%x\n", seed);
}

BN_CTX* init_ctx() {
  // XXX: Is there more initialization to do?
  return BN_CTX_new();
}

// TODO: use BN_CTX_get(ctx) and properly initialize the bignum
// or: use BN_new() to get a bignum
BIGNUM* init_bn(BN_CTX* ctx) {
  return BN_CTX_get(ctx);
}

void nd_ulong(BN_ULONG *ul) {
  int *p = (int*) ul;
  for (int i = 0; i < (sizeof(*ul) / sizeof(int)); ++i) {
    *p = rand();
    ++p;
  }
}

int nd_int() {
  return rand();
}

void start_clock() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  tstart = 1000.0 * ts.tv_sec + 1e-6 * ts.tv_nsec;
}

void end_clock() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double tend = 1000.0 * ts.tv_sec + 1e-6 * ts.tv_nsec;
  // diff in ms
  fprintf(stderr, "clock diff: %.2f\n", tend - tstart);
}

void display_ctx(BN_CTX* ctx) {
  printf("ctx found\n");
}

void display_bn(BIGNUM* bn) {
  if (bn->neg) { printf("-"); }
  int len = bn->dmax;
  BN_ULONG *d = bn->d;
  for (int i = 0; i < len; ++i) {
    printf("%lu, ", d[i]);
  }
  printf("\n");
}

void display_ulong(BN_ULONG* ul) {
  printf("%lu\n", *ul);
}

void display_int(int i) {
  printf("%d\n", i);
}

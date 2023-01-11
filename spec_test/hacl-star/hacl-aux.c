#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "hacl-aux.h"

// print results (big file)
#define PRINT
char output_filename[] = "ecdh-output.txt";

#define MSG_SIZE (1024 * 1024 * 16)

unsigned seed;
double tstart = 0.0;

void init_seed(char** arg) {
  seed = strtoul(arg[1], NULL, 16);
  srand(seed);
#ifdef PRINT
  printf("seed: 0x%x\n", seed);
#endif
}

size_t get_length() {
  return MSG_SIZE;
}

uint8_t *init_msg() {
  uint8_t *msg = malloc(MSG_SIZE);
  int *p = (int*) msg;
  for (unsigned i = 0; i < MSG_SIZE / sizeof(int); ++i) {
    // put "random" values into 'msg'
    p[i] = rand();
  }
  return msg;
}

void init_key(uint8_t key[32]) {
  int *p = (int*) key;
  for (unsigned i = 0; i < 32 / sizeof(int); ++i) {
    // put "random" values into 'a'
    p[i] = rand();
  }
}

void init(uint8_t a[32U]) {
  int *p = (int*) a;
  for (unsigned i = 0; i < 32U / sizeof(int); ++i) {
    // put "random" values into 'a'
    p[i] = rand();
  }
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
  fprintf(stderr, "  clock diff: %.1f\n", tend - tstart);
}

void display_signature(uint8_t signature[64]) {
#ifdef PRINT
  FILE *f = fopen(output_filename, "a");
  fprintf(f, "encrypted: 0x%.2x", signature[0]);
  for (unsigned i = 1; i < 64; ++i) {
    fprintf(f, " %.2x", signature[i]);
  }
  fprintf(f, "\n");
  fclose(f);
#endif
}

void display(uint8_t out[32U]) {
#ifdef PRINT
  FILE *f = fopen(output_filename, "a");
  fprintf(f, "encrypted: 0x%.2x", out[0]);
  for (unsigned i = 1; i < 32U; ++i) {
    fprintf(f, " %.2x", out[i]);
  }
  fprintf(f, "\n");
  fclose(f);
#endif
}

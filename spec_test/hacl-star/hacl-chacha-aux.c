#include <stdio.h>
#include <time.h>

#include "openssl-chacha-aux.h"

// print results (big file)
//#define PRINT
char output_filename[] = "chacha-output.txt";

#define ARRAY_SIZE (1024 * 1024 * 16)

unsigned seed;
//unsigned char in[ARRAY_SIZE];
//unsigned char out[ARRAY_SIZE] = {0};
unsigned char *in;
unsigned char *out;
double tstart = 0.0;

void init_seed(char** arg) {
  seed = strtoul(arg[1], NULL, 16);
  srand(seed);
#ifdef PRINT
  printf("seed: 0x%x\n", seed);
#endif
}

size_t get_length() {
  return ARRAY_SIZE;
}

unsigned char *init_array(bool is_in) {
  if (is_in) {
    in = malloc(ARRAY_SIZE);
    int *p = (int*) in;
    for (unsigned i = 0; i < ARRAY_SIZE / sizeof(int); ++i) {
      // put "random" values into 'in'
      p[i] = rand();
    }
    return in;
  } else {
    out = calloc(1, ARRAY_SIZE);
    return out;
  }
}

// TODO
uint8_t *init_key();

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

void display_chacha(unsigned char *out) {
#ifdef PRINT
  FILE *f = fopen(output_filename, "a");
  fprintf(f, "encrypted: 0x%.2x", out[0]);
  for (unsigned i = 1; i < ARRAY_SIZE; ++i) {
    fprintf(f, " %.2x", out[i]);
  }
  fprintf(f, "\n");
  fclose(f);
#endif
}

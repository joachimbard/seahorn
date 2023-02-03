#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "hacl-poly-aux.h"

// print results (big file)
//#define PRINT
char output_filename[] = "poly-output.txt";

#define ARRAY_SIZE (1024 * 1024 * 256)

unsigned seed;
//unsigned char in[ARRAY_SIZE];
//unsigned char out[ARRAY_SIZE] = {0};
uint8_t *in;
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

uint8_t *init_array() {
  in = malloc(ARRAY_SIZE);
  int *p = (int*) in;
  for (unsigned i = 0; i < ARRAY_SIZE / sizeof(int); ++i) {
    // put "random" values into 'in'
    p[i] = rand();
  }
  return in;
}

void init_key(uint8_t *key) {
  int *p = (int*) key;
  for (unsigned i = 0; i < KEY_SIZE / sizeof(int); ++i) {
    // put "random" values into 'key'
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

void print_array() {
  FILE *f = fopen(output_filename, "a");
  fprintf(f, "array: 0x%.2x", in[0]);
  for (unsigned i = 1; i < ARRAY_SIZE; ++i) {
    fprintf(f, " %.2x", in[i]);
  }
  fprintf(f, "\n");
  fclose(f);
}

void display_poly(uint8_t *tag) {
#ifdef PRINT
//  print_array();
  FILE *f = fopen(output_filename, "a");
  fprintf(f, "encrypted: 0x%.2x", tag[0]);
  for (unsigned i = 1; i < TAG_SIZE; ++i) {
    fprintf(f, " %.2x", tag[i]);
  }
  fprintf(f, "\n");
  fclose(f);
#endif
}

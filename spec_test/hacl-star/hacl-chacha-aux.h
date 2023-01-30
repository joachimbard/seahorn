#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define N_SIZE (12)
#define KEY_SIZE (32)

void init_seed(char**);
size_t get_length();
uint8_t *init_array(bool);
void start_clock();
void end_clock();
void init_key(uint8_t*);
void init_n(uint8_t*);
uint32_t init_ctr();
void display_chacha(unsigned char*);

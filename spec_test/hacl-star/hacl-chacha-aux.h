#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void init_seed(char**);
size_t get_length();
uint8_t *init_array(bool);
void start_clock();
void end_clock();
uint8_t *init_key();
uint32_t init_ctr();
void display_chacha(unsigned char*);

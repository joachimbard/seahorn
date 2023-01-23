#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void init_seed(char**);
uint8_t *init_tag();
size_t get_length();
uint8_t *init_array(bool);
void start_clock();
void end_clock();
uint8_t *init_key();
void display_poly(unsigned char*);

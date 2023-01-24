#include <stddef.h>
#include <stdint.h>

#define TAG_SIZE (16)
#define KEY_SIZE (32)

void init_seed(char**);
uint8_t *init_tag();
size_t get_length();
uint8_t *init_array();
void start_clock();
void end_clock();
void init_key(uint8_t*);
void display_poly(uint8_t*);

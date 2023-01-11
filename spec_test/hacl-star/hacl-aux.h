#include <stdint.h>
#include <stddef.h>

void init_seed(char**);
void start_clock();
void end_clock();

size_t get_length();
uint8_t *init_msg();
void init_key(uint8_t key[32]);
void display_signature(uint8_t signature[64]);

void init(uint8_t a[32U]);
void display(uint8_t out[32U]);


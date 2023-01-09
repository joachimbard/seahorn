#include <stdbool.h>

#ifdef OO7
int get_taint_source();
#endif

void init_seed(char**);
size_t get_length();
unsigned char *init_array(bool);
unsigned char *init_ivec();
void start_clock();
void end_clock();
void init_key(AES_KEY*);
void display_aes(unsigned char*);

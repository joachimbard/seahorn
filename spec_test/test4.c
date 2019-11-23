/* EXAMPLE 4:  Add a left shift by one on the index. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <seahorn/seahorn.h>

extern void __taint(int*);
extern void __is_tainted(int);
extern void spec_fence();
extern int nd();

unsigned int array1_size = 16;
uint8_t array1[16];
uint8_t array2[256 * 512];
uint8_t temp = 0;

void main(int argn, char* args[]) {
    int source;

    source = nd();
    __taint(&source);
    if (source < array1_size) {                  
        temp &= array2[array1[source << 1] * 512];
    }
}

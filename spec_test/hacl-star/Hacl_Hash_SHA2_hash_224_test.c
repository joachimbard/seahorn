// We include the implementation on purpose
#include "Hacl_Hash_SHA2.c"

extern uint8_t *init();
extern uint32_t init_len();
extern void display(uint8_t *dst);

int main() {
  uint8_t *input = init();
  uint32_t input_len = init_len();
  uint8_t *dst = init();

  Hacl_Hash_SHA2_hash_224(input, input_len, dst);

  display(dst);

  return 0;
}

#include "hacl-poly-aux.h"

#define HACL_CAN_COMPILE_VEC256 1
// We include the implementation on purpose
#include "Hacl_Poly1305_256.c"

int main(int argc, char **argv) {
  init_seed(argv);
  uint8_t tag[TAG_SIZE] = { 0 };
  uint32_t len = get_length();
  uint8_t *text = init_array();
  uint8_t key[KEY_SIZE];
  init_key(key);

  start_clock();
  Hacl_Poly1305_256_poly1305_mac(tag, len, text, key);
  end_clock();

  display_poly(tag);

  return 0;
}

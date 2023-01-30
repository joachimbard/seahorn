#include "hacl-chacha-aux.h"

// We include the implementation on purpose
#include "Hacl_Chacha20.c"

int main(int argc, char **argv) {
  init_seed(argv);
  uint32_t len = get_length();
  uint8_t *out = init_array(false);
  uint8_t *text = init_array(true);
  uint8_t key[KEY_SIZE];
  init_key(key);
  uint8_t n[N_SIZE];
  init_n(n);
  uint32_t ctr = init_ctr();

  start_clock();
  Hacl_Chacha20_chacha20_encrypt(len, out, text, key, n, ctr);
  end_clock();

  display_chacha(out);

  return 0;
}

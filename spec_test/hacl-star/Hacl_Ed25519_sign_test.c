#include "hacl-aux.h"

// We include the implementation on purpose
#include "Hacl_Ed25519.c"

int main(int argc, char **argv) {
  init_seed(argv);
  uint8_t signature[64] = {0};
  uint8_t private_key[32];
  init_key(private_key);
  uint32_t msg_len = get_length();
  uint8_t *msg = init_msg();

  start_clock();
  Hacl_Ed25519_sign(signature, private_key, msg_len, msg);
  end_clock();

  display_signature(signature);

  return 0;
}

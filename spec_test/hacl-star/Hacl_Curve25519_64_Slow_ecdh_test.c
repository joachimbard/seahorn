#include "hacl-aux.h"

// We include the implementation on purpose
#include "Hacl_Curve25519_64_Slow.c"

int main(int argc, char **argv) {
  init_seed(argv);
  uint8_t out[32U];
  uint8_t priv[32U];
  uint8_t pub[32U];

  init(out);
  init(priv);
  init(pub);

  start_clock();
  Hacl_Curve25519_64_Slow_ecdh(out, priv, pub);
  end_clock();

  display(out);

  return 0;
}

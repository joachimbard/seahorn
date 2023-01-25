#include "hacl-aux.h"

//#define HACL_CAN_COMPILE_INLINE_ASM 1
// We include the implementation on purpose
#include "Hacl_Curve25519_51.c"

int main(int argc, char **argv) {
  init_seed(argv);
  uint8_t out[32U] = {0};
  uint8_t priv[32U];
  uint8_t pub[32U];

//  init(out);
  init(priv);
  init(pub);

  start_clock();
  Hacl_Curve25519_51_ecdh(out, priv, pub);
  end_clock();

  display(out);

  return 0;
}

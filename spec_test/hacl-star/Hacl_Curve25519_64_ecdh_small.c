// We include the implementation on purpose
#include "Hacl_Curve25519_64_small.c"

extern void init(uint8_t a[32U]);
extern void display(uint8_t a[32U]);

int main() {
  uint8_t out[32U];
  uint8_t priv[32U];
  uint8_t pub[32U];

  init(out);
  init(priv);
  init(pub);

  Hacl_Curve25519_64_ecdh(out, priv, pub);

  display(out);

//  for (size_t i = 0; i < sizeof(out / sizeof(*out)); ++i) {
//    printf("%c", out[i]);
//  }
//  printf("\n");

  return 0;
}

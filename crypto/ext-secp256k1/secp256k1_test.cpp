#include "secp256k1.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <gmp.h>

#define WITH_MPZ(body, ...)                                                    \
  mpz_t __VA_ARGS__;                                                           \
  mpz_inits(__VA_ARGS__, NULL);                                                \
  body mpz_clears(__VA_ARGS__, NULL);

int main() {
  SECP256K1::init(time(NULL));
  WITH_MPZ(
      {
        SECP256K1KEY *key1 = SECP256K1::gen();
        SECP256K1PK *pk1 = new SECP256K1PK(*key1);
        SECP256K1KEY *key2 = SECP256K1::gen();
        SECP256K1PK *pk2 = new SECP256K1PK(*key2);
        key1->exchange(x1, y1, *pk2);
        key2->exchange(x2, y2, *pk1);
        assert(!mpz_cmp(x1, x2) && !mpz_cmp(y1, y2));
        gmp_printf("key(does this like random?): %Zx\n", x1);
        SECP256K1::urandom(h);
        key1->sign(r, s, h);
        assert(!pk1->verify(r, s, h) && pk2->verify(r, s, h));
      },
      x1, y1, x2, y2, h, r, s);
  SECP256K1::clear();
}

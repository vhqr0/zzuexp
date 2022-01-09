#include "rsa.h"

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
  RSA rsa(time(NULL));
  RSAKEY *key = rsa.gen();
  RSAPK *pk = new RSAPK(*key);
  WITH_MPZ(
      {
        rsa.urandom(plain);
        pk->encrypt(cipher, plain);
        key->decrypt(dcipher, cipher);
        assert(!mpz_cmp(plain, dcipher));
      },
      plain, cipher, dcipher);
  delete pk;
  delete key;
}

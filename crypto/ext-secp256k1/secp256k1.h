#ifndef SECP256K1_H
#define SECP256K1_H

#include <stdarg.h>
#include <stdio.h>

#include <gmp.h>

class SECP256K1KEY;

class SECP256K1 {
public:
  static void init(unsigned int seed);
  static void clear();
  static void urandom(mpz_t x);
  static SECP256K1KEY *gen();

protected:
  static mpz_t prime, order, basex, basey;

  static void add_eq(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1,
                     mpz_t z1);
  static void add(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1, mpz_t z1,
                  mpz_t x2, mpz_t y2, mpz_t z2);
  static void sub(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1, mpz_t z1,
                  mpz_t x2, mpz_t y2, mpz_t z2);
  static void mul(mpz_t x2, mpz_t y2, mpz_t z2, mpz_t x1, mpz_t y1, mpz_t z1,
                  mpz_t k);
  static void mapping(mpz_t x, mpz_t y, mpz_t X, mpz_t Y, mpz_t Z);

private:
  static gmp_randstate_t rand_;
};

class SECP256K1PK : public SECP256K1 {
  friend class SECP256K1KEY;

public:
  SECP256K1PK(mpz_t k);
  SECP256K1PK(mpz_t x, mpz_t y);
  SECP256K1PK(SECP256K1PK &pk);
  ~SECP256K1PK();
  int verify(mpz_t r, mpz_t s, mpz_t h);

private:
  mpz_t x_, y_;
};

class SECP256K1KEY : public SECP256K1PK {
public:
  SECP256K1KEY(mpz_t k);
  ~SECP256K1KEY();
  void exchange(mpz_t x, mpz_t y, SECP256K1PK &pk);
  void sign(mpz_t r, mpz_t s, mpz_t h);

private:
  mpz_t k_;
};

#endif

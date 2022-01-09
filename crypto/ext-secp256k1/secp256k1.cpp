#include "secp256k1.h"

#include <stdarg.h>
#include <stdio.h>

#include <gmp.h>

#define WITH_MPZ(body, ...)                                                    \
  mpz_t __VA_ARGS__;                                                           \
  mpz_inits(__VA_ARGS__, NULL);                                                \
  body mpz_clears(__VA_ARGS__, NULL);

#define PRIME                                                                  \
  "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF"                                        \
  "FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F"

#define ORDER                                                                  \
  "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE"                                        \
  "BAAEDCE6 AF48A03B BFD25E8C D0364141"

#define BASEX                                                                  \
  "79BE667E F9DCBBAC 55A06295 CE870B07"                                        \
  "029BFCDB 2DCE28D9 59F2815B 16F81798"

#define BASEY                                                                  \
  "483ADA77 26A3C465 5DA4FBFC 0E1108A8"                                        \
  "FD17B448 A6855419 9C47D08F FB10D4B8"

mpz_t SECP256K1::prime, SECP256K1::order, SECP256K1::basex, SECP256K1::basey;

gmp_randstate_t SECP256K1::rand_;

void SECP256K1::init(unsigned int seed) {
  mpz_init_set_str(prime, PRIME, 16);
  mpz_init_set_str(order, ORDER, 16);
  mpz_init_set_str(basex, BASEX, 16);
  mpz_init_set_str(basey, BASEY, 16);
  gmp_randinit_default(rand_);
  gmp_randseed_ui(rand_, seed);
}

void SECP256K1::clear() {
  mpz_clears(prime, order, basex, basey, NULL);
  gmp_randclear(rand_);
}

void SECP256K1::urandom(mpz_t x) { mpz_urandomm(x, rand_, order); }

SECP256K1KEY *SECP256K1::gen() {
  SECP256K1KEY *key;
  WITH_MPZ(
      {
        urandom(k);
        key = new SECP256K1KEY(k);
      },
      k);
  return key;
}

SECP256K1PK::SECP256K1PK(mpz_t k) {
  mpz_inits(x_, y_, NULL);
  WITH_MPZ(
      {
        mpz_set_ui(basez, 1);
        mul(x, y, z, basex, basey, basez, k);
        mapping(x_, y_, x, y, z);
      },
      x, y, z, basez);
}

SECP256K1PK::SECP256K1PK(mpz_t x, mpz_t y) {
  mpz_init_set(x_, x);
  mpz_init_set(y_, y);
}

SECP256K1PK::SECP256K1PK(SECP256K1PK &pk) {
  mpz_init_set(x_, pk.x_);
  mpz_init_set(y_, pk.y_);
}

SECP256K1PK::~SECP256K1PK() { mpz_clears(x_, y_, NULL); }

SECP256K1KEY::SECP256K1KEY(mpz_t k) : SECP256K1PK(k) { mpz_init_set(k_, k); }

SECP256K1KEY::~SECP256K1KEY() { mpz_clear(k_); }

void SECP256K1::add_eq(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1,
                       mpz_t z1) {
  WITH_MPZ(
      {
        mpz_powm_ui(s0, x1, 2, prime);
        mpz_mul_ui(s0, s0, 3);
        mpz_mod(s0, s0, prime);
        mpz_powm_ui(s1, s0, 2, prime);
        mpz_powm_ui(s2, y1, 2, prime);
        mpz_mul(s3, s2, x1);
        mpz_mod(s3, s3, prime);
        mpz_mul_ui(s3, s3, 4);
        mpz_mod(s3, s3, prime);
        mpz_mul_ui(s4, s3, 2);
        mpz_mod(s4, s4, prime);
        mpz_powm_ui(s2, s2, 2, prime);
        mpz_mul_ui(s2, s2, 8);
        mpz_mod(s2, s2, prime);
        mpz_sub(s1, s1, s4);
        mpz_mod(s1, s1, prime);
        mpz_sub(s3, s3, s1);
        mpz_mod(s3, s3, prime);
        mpz_mul(s0, s0, s3);
        mpz_sub(s0, s0, s2);
        mpz_mod(s0, s0, prime);
        mpz_mul(s2, y1, z1);
        mpz_mod(s2, s2, prime);
        mpz_mul_ui(s2, s2, 2);
        mpz_mod(s2, s2, prime);
        mpz_swap(x3, s1);
        mpz_swap(y3, s0);
        mpz_swap(z3, s2);
      },
      s0, s1, s2, s3, s4);
}

void SECP256K1::add(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1, mpz_t z1,
                    mpz_t x2, mpz_t y2, mpz_t z2) {
  if (!mpz_cmp_ui(z1, 0)) {
    mpz_set(x3, x2);
    mpz_set(y3, y2);
    mpz_set(z3, z2);
    return;
  }
  if (!mpz_cmp_ui(z2, 0)) {
    mpz_set(x3, x1);
    mpz_set(y3, y1);
    mpz_set(z3, z1);
  }
  WITH_MPZ(
      {
        mpz_powm_ui(t1, z2, 2, prime);
        mpz_mul(t1, t1, x1);
        mpz_mod(t1, t1, prime);
        mpz_powm_ui(t2, z1, 2, prime);
        mpz_mul(t2, t2, x2);
        mpz_mod(t2, t2, prime);
        if (!mpz_cmp(t1, t2)) {
          mpz_powm_ui(t1, z2, 3, prime);
          mpz_mul(t1, t1, y1);
          mpz_mod(t1, t1, prime);
          mpz_powm_ui(t2, z1, 3, prime);
          mpz_mul(t2, t2, y2);
          mpz_mod(t2, t2, prime);
          if (!mpz_cmp(t1, t2)) {
            add_eq(x3, y3, z3, x1, y1, z1);
          } else {
            mpz_set_ui(x3, 0);
            mpz_set_ui(y3, 0);
            mpz_set_ui(z3, 0);
          }
          mpz_clears(t1, t2, NULL);
          return;
        }
      },
      t1, t2);
  WITH_MPZ(
      {
        mpz_powm_ui(s0, z1, 2, prime);
        mpz_mul(s1, s0, x2);
        mpz_mod(s1, s1, prime);
        mpz_mul(s0, s0, z1);
        mpz_mod(s0, s0, prime);
        mpz_mul(s0, s0, y2);
        mpz_mod(s0, s0, prime);
        mpz_powm_ui(s2, z2, 2, prime);
        mpz_mul(s3, s2, x1);
        mpz_mod(s3, s3, prime);
        mpz_mul(s2, s2, z2);
        mpz_mod(s2, s2, prime);
        mpz_mul(s2, s2, y1);
        mpz_mod(s2, s2, prime);
        mpz_sub(s0, s0, s2);
        mpz_mod(s0, s0, prime);
        mpz_sub(s1, s1, s3);
        mpz_mod(s1, s1, prime);
        mpz_mul(s4, z1, z2);
        mpz_mod(s4, s4, prime);
        mpz_mul(s4, s4, s1);
        mpz_mod(s4, s4, prime);
        mpz_powm_ui(s5, s1, 2, prime);
        mpz_mul(s3, s3, s5);
        mpz_mod(s3, s3, prime);
        mpz_mul(s1, s1, s5);
        mpz_mod(s1, s1, prime);
        mpz_mul(s2, s2, s1);
        mpz_mod(s2, s2, prime);
        mpz_powm_ui(s5, s0, 2, prime);
        mpz_sub(s5, s5, s1);
        mpz_submul_ui(s5, s3, 2);
        mpz_mod(s5, s5, prime);
        mpz_sub(s3, s3, s5);
        mpz_mod(s3, s3, prime);
        mpz_mul(s0, s0, s3);
        mpz_sub(s0, s0, s2);
        mpz_mod(s0, s0, prime);
        mpz_swap(x3, s5);
        mpz_swap(y3, s0);
        mpz_swap(z3, s4);
      },
      s0, s1, s2, s3, s4, s5);
}

void SECP256K1::sub(mpz_t x3, mpz_t y3, mpz_t z3, mpz_t x1, mpz_t y1, mpz_t z1,
                    mpz_t x2, mpz_t y2, mpz_t z2) {
  WITH_MPZ(
      {
        mpz_sub(t, prime, y2);
        add(x3, y3, z3, x1, y1, z1, x2, t, z2);
      },
      t);
}

void SECP256K1::mul(mpz_t x2, mpz_t y2, mpz_t z2, mpz_t x1, mpz_t y1, mpz_t z1,
                    mpz_t k) {
  WITH_MPZ(
      {
        mpz_set(_k, k);
        mpz_set(bx, x1);
        mpz_set(by, y1);
        mpz_set(bz, z1);
        while (mpz_cmp_ui(_k, 0)) {
          mpz_tdiv_r_2exp(_r, _k, 1);
          mpz_tdiv_q_2exp(_k, _k, 1);
          if (mpz_cmp_ui(_r, 0)) {
            mpz_tdiv_r_2exp(_r, _k, 1);
            if (mpz_cmp_ui(_r, 0)) {
              mpz_add_ui(_k, _k, 1);
              sub(ax, ay, az, ax, ay, az, bx, by, bz);
            } else {
              add(ax, ay, az, ax, ay, az, bx, by, bz);
            }
          }
          add_eq(bx, by, bz, bx, by, bz);
        }
        mpz_swap(x2, ax);
        mpz_swap(y2, ay);
        mpz_swap(z2, az);
      },
      _k, _r, ax, ay, az, bx, by, bz);
}

void SECP256K1::mapping(mpz_t x, mpz_t y, mpz_t X, mpz_t Y, mpz_t Z) {
  WITH_MPZ(
      {
        if (!mpz_cmp_ui(Z, 0)) {
          mpz_set_ui(x, 0);
          mpz_set_ui(y, 0);
        } else {
          mpz_powm_ui(t1, Z, 2, prime);
          mpz_invert(t1, t1, prime);
          mpz_mul(t1, t1, X);
          mpz_mod(t1, t1, prime);
          mpz_powm_ui(t2, Z, 3, prime);
          mpz_invert(t2, t2, prime);
          mpz_mul(t2, t2, Y);
          mpz_mod(t2, t2, prime);
          mpz_swap(x, t1);
          mpz_swap(y, t2);
        }
      },
      t1, t2);
}

void SECP256K1KEY::exchange(mpz_t x, mpz_t y, SECP256K1PK &pk) {
  WITH_MPZ(
      {
        mpz_set_ui(z_, 1);
        mul(X, Y, Z, pk.x_, pk.y_, z_, k_);
        mapping(x, y, X, Y, Z);
      },
      z_, X, Y, Z);
}

void SECP256K1KEY::sign(mpz_t r, mpz_t s, mpz_t h) {
  WITH_MPZ(
      {
        urandom(k);
        mpz_set_ui(z, 1);
        mul(x, y, z, basex, basey, z, k);
        mapping(x, y, x, y, z);
        mpz_invert(k, k, order);
        mpz_mul(y, x, k_);
        mpz_add(y, y, h);
        mpz_mod(y, y, order);
        mpz_mul(y, y, k);
        mpz_mod(y, y, order);
        mpz_swap(r, x);
        mpz_swap(s, y);
      },
      k, x, y, z);
}

int SECP256K1PK::verify(mpz_t r, mpz_t s, mpz_t h) {
  int t;
  WITH_MPZ(
      {
        mpz_set_ui(z1, 1);
        mpz_set_ui(z2, 1);
        mul(x1, y1, z1, basex, basey, z1, h);
        mul(x2, y2, z2, x_, y_, z2, r);
        add(x1, y1, z1, x1, y1, z1, x2, y2, z2);
        mpz_invert(x2, s, order);
        mul(x1, y1, z1, x1, y1, z1, x2);
        mapping(x1, y1, x1, y1, z1);
        t = mpz_cmp(x1, r);
      },
      x1, y1, z1, x2, y2, z2);
  return t;
}

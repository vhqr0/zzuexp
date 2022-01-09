#include "rsa.h"

#include <stdarg.h>
#include <stdio.h>

#include <gmp.h>

#define WITH_MPZ(body, ...)                                                    \
  mpz_t __VA_ARGS__;                                                           \
  mpz_inits(__VA_ARGS__, NULL);                                                \
  body mpz_clears(__VA_ARGS__, NULL);

RSAPK::RSAPK(mpz_t p, mpz_t q, mpz_t e) {
  mpz_inits(n_, e_, NULL);
  mpz_mul(n_, p, q);
  mpz_set(e_, e);
}

RSAPK::RSAPK(mpz_t n, mpz_t e) {
  mpz_init_set(n_, n);
  mpz_init_set(e_, e);
}

RSAPK::RSAPK(RSAPK &pk) {
  mpz_init_set(n_, pk.n_);
  mpz_init_set(e_, pk.e_);
}

RSAPK::~RSAPK() { mpz_clears(n_, e_, NULL); }

void RSAPK::encrypt(mpz_t cipher, mpz_t plain) {
  mpz_powm(cipher, plain, e_, n_);
}

RSAKEY::RSAKEY(mpz_t p, mpz_t q, mpz_t e) : RSAPK(p, q, e) {
  WITH_MPZ(
      {
        mpz_inits(p_, q_, dp_, dq_, qinv_, NULL);
        mpz_set(p_, p);
        mpz_set(q_, q);
        mpz_invert(qinv_, q, p);
        mpz_sub_ui(p, p, 1);
        mpz_sub_ui(q, q, 1);
        mpz_lcm(d, p, q);
        mpz_invert(d, e, d);
        mpz_mod(dp_, d, p);
        mpz_mod(dq_, d, q);
      },
      d);
}

RSAKEY::~RSAKEY() { mpz_clears(p_, q_, dp_, dq_, qinv_, NULL); }

void RSAKEY::decrypt(mpz_t plain, mpz_t cipher) {
  WITH_MPZ(
      {
        mpz_powm(plain, cipher, dq_, q_);
        mpz_powm(h, cipher, dp_, p_);
        mpz_sub(h, h, plain);
        mpz_mod(h, h, p_);
        mpz_mul(h, h, qinv_);
        mpz_mod(h, h, p_);
        mpz_mul(h, h, q_);
        mpz_add(plain, plain, h);
      },
      h);
}

RSA::RSA(unsigned int seed, unsigned int length, unsigned int e)
    : length_(length >> 1) {
  gmp_randinit_default(rand_);
  gmp_randseed_ui(rand_, seed);
  mpz_init_set_ui(e_, e);
}

RSA::~RSA() {
  gmp_randclear(rand_);
  mpz_clears(e_);
}

void RSA::urandom(mpz_t x) { mpz_urandomb(x, rand_, length_); }

RSAKEY *RSA::gen() {
  RSAKEY *key;
  WITH_MPZ(
      {
        urandom(p);
        urandom(q);
        do {
          mpz_nextprime(p, p);
          mpz_mod(t, p, e_);
        } while (!mpz_cmp_ui(t, 1));
        do {
          do {
            mpz_nextprime(q, q);
            mpz_mod(t, q, e_);
          } while (!mpz_cmp_ui(t, 1));
        } while (!mpz_cmp(p, q));
        key = new RSAKEY(p, q, e_);
      },
      p, q, t);
  return key;
}

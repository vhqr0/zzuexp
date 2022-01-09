#ifndef RSA_H
#define RSA_H

#include <stdarg.h>
#include <stdio.h>

#include <gmp.h>

class RSAPK {
public:
  RSAPK(mpz_t p, mpz_t q, mpz_t e);
  RSAPK(mpz_t n, mpz_t e);
  RSAPK(RSAPK &pk);
  ~RSAPK();
  void encrypt(mpz_t cipher, mpz_t plain);

private:
  mpz_t n_, e_;
};

class RSAKEY : public RSAPK {
public:
  RSAKEY(mpz_t p, mpz_t q, mpz_t e);
  ~RSAKEY();
  void decrypt(mpz_t plain, mpz_t cipher);

private:
  mpz_t p_, q_, dp_, dq_, qinv_;
};

class RSA {
public:
  RSA(unsigned int seed, unsigned int length = 2048, unsigned int e = 65537);
  ~RSA();
  void urandom(mpz_t x);
  RSAKEY *gen();

private:
  unsigned int length_;
  mpz_t e_;
  gmp_randstate_t rand_;
};

#endif

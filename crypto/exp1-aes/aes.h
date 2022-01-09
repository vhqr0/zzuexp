#ifndef AES_H
#define AES_H

#include <stdint.h>

class AES {
public:
  AES(const uint8_t *key, int size);
  void encrypt(uint32_t *buf);
  void decrypt(uint32_t *buf);

private:
  static const uint8_t rcon[10];
  static const uint8_t sbox[256], sbox_inv[256];
  static const uint8_t m2[256], m3[256], m9[256], mb[256], md[256], me[256];

  int Nk_, Nr_;
  uint32_t key_[60];

  static void subbytes(uint8_t *a);
  static void subbytes_inv(uint8_t *a);
  static void shiftrows(uint8_t *a);
  static void shiftrows_inv(uint8_t *a);
  static void mixcolumns(uint8_t *a);
  static void mixcolumns_inv(uint8_t *a);
  void addroundkey(uint32_t *buf, int r);
};

class AESCTR : public AES {
public:
  AESCTR(const uint8_t *key, int size, const uint8_t *iv, uint32_t ctr = 1);
  void ek(uint32_t *buf);
  uint8_t *ctr(const uint8_t *plain, int size);

private:
  uint32_t ctr_;
  uint8_t iv_[12];
};

class AESGCM : public AESCTR {
public:
  AESGCM(const uint8_t *key, int size, const uint8_t *iv);
  void gcm(const uint8_t *add, int add_size, const uint8_t *plain,
           int plain_size, uint8_t *&cipher, uint8_t *&tag);

private:
  uint64_t h_[2];
  uint64_t tag_[2];
  uint64_t ek1_[2];

  void gmul();
  void update_tag(const uint8_t *data, int size);
};

#endif

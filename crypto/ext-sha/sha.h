#ifndef SHA_H
#define SHA_H

#include <stdint.h>

// Bits :  32 |  64
// Size : 256 | 512
#define SHA_DEC(B, S)                                                          \
  class SHA##S {                                                               \
  public:                                                                      \
    uint##B##_t hash[8];                                                       \
    SHA##S();                                                                  \
    void init();                                                               \
    void update(uint##B##_t *buf);                                             \
    void finish(uint##B##_t *buf, int size, int count);                        \
    void sha(const uint8_t *buf, int size);                                    \
                                                                               \
  private:                                                                     \
    static const uint##B##_t init_hash[8];                                     \
    static const uint##B##_t key[S == 256 ? 64 : 80];                          \
  };                                                                           \
                                                                               \
  class SHAHMAC##S {                                                           \
  public:                                                                      \
    uint##B##_t tag[8];                                                        \
    SHAHMAC##S(const uint8_t *key, int size);                                  \
    void hmac(const uint8_t *buf, int size);                                   \
                                                                               \
  private:                                                                     \
    uint##B##_t okey_[16];                                                     \
    uint##B##_t ikey_[16];                                                     \
  };

SHA_DEC(32, 256);
SHA_DEC(64, 512);

#undef SHA_DEC

#endif

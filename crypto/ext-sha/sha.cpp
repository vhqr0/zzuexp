#include "sha.h"

#include <endian.h>
#include <stdint.h>

#include <cstring>

const uint32_t SHA256::init_hash[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

const uint64_t SHA512::init_hash[8] = {
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b,
    0xa54ff53a5f1d36f1, 0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};

const uint32_t SHA256::key[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

const uint64_t SHA512::key[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f,
    0xe9b5dba58189dbbc, 0x3956c25bf348b538, 0x59f111f1b605d019,
    0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242,
    0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
    0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3,
    0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 0x2de92c6f592b0275,
    0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f,
    0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
    0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc,
    0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6,
    0x92722c851482353b, 0xa2bfe8a14cf10364, 0xa81a664bbc423001,
    0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
    0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99,
    0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb,
    0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc,
    0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915,
    0xc67178f2e372532b, 0xca273eceea26619c, 0xd186b8c721c0c207,
    0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba,
    0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
    0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a,
    0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};

#define INLINE static inline

// Bits :       32 |       64
// Size :      256 |      512
// A    :   7,18,3 |    1,8,7
// B    : 17,19,10 |  19,61,6
// C    :  2,13,22 | 28,34,39
// D    :  6,11,25 | 14,18,41
#define SHA_IMP(B, S, A0, A1, A2, B0, B1, B2, C0, C1, C2, D0, D1, D2)          \
  INLINE uint##B##_t ch##B(uint##B##_t x, uint##B##_t y, uint##B##_t z) {      \
    return z ^ (x & (y ^ z));                                                  \
  }                                                                            \
  INLINE uint##B##_t maj##B(uint##B##_t x, uint##B##_t y, uint##B##_t z) {     \
    return (x & y) | (z & (x | y));                                            \
  }                                                                            \
  INLINE uint##B##_t rr##B(uint##B##_t x, int n) {                             \
    return x >> n | (x << (B - n));                                            \
  }                                                                            \
  INLINE uint##B##_t t0##B(uint##B##_t x) {                                    \
    return rr##B(x, A0) ^ rr##B(x, A1) ^ (x >> A2);                            \
  }                                                                            \
  INLINE uint##B##_t t1##B(uint##B##_t x) {                                    \
    return rr##B(x, B0) ^ rr##B(x, B1) ^ (x >> B2);                            \
  }                                                                            \
  INLINE uint##B##_t s0##B(uint##B##_t x) {                                    \
    return rr##B(x, C0) ^ rr##B(x, C1) ^ rr##B(x, C2);                         \
  }                                                                            \
  INLINE uint##B##_t s1##B(uint##B##_t x) {                                    \
    return rr##B(x, D0) ^ rr##B(x, D1) ^ rr##B(x, D2);                         \
  }                                                                            \
  SHA##S::SHA##S() { init(); }                                                 \
  void SHA##S::init() {                                                        \
    for (int i = 0; i < 8; i++)                                                \
      hash[i] = init_hash[i];                                                  \
  }                                                                            \
  void SHA##S::update(uint##B##_t *buf) {                                      \
    uint##B##_t a, b, c, d, e, f, g, h;                                        \
    a = hash[0];                                                               \
    b = hash[1];                                                               \
    c = hash[2];                                                               \
    d = hash[3];                                                               \
    e = hash[4];                                                               \
    f = hash[5];                                                               \
    g = hash[6];                                                               \
    h = hash[7];                                                               \
    for (int i = 0; i < (S == 256 ? 64 : 80); i++) {                           \
      if (i < 16) {                                                            \
        buf[i] = be##B##toh(buf[i]);                                           \
      } else {                                                                 \
        uint##B##_t t0 = t0##B(buf[(i - 15) % 16]);                            \
        uint##B##_t t1 = t1##B(buf[(i - 2) % 16]);                             \
        buf[i % 16] += buf[(i - 7) % 16] + t0 + t1;                            \
      }                                                                        \
      uint##B##_t s0 = s0##B(a);                                               \
      uint##B##_t s1 = s1##B(e);                                               \
      uint##B##_t ch = ch##B(e, f, g);                                         \
      uint##B##_t maj = maj##B(a, b, c);                                       \
      uint##B##_t t1 = h + s1 + ch + buf[i % 16] + key[i];                     \
      uint##B##_t t2 = s0 + maj;                                               \
      h = g;                                                                   \
      g = f;                                                                   \
      f = e;                                                                   \
      e = d + t1;                                                              \
      d = c;                                                                   \
      c = b;                                                                   \
      b = a;                                                                   \
      a = t1 + t2;                                                             \
    }                                                                          \
    hash[0] += a;                                                              \
    hash[1] += b;                                                              \
    hash[2] += c;                                                              \
    hash[3] += d;                                                              \
    hash[4] += e;                                                              \
    hash[5] += f;                                                              \
    hash[6] += g;                                                              \
    hash[7] += h;                                                              \
  }                                                                            \
  void SHA##S::finish(uint##B##_t *buf, int size, int count) {                 \
    uint64_t length = (((uint64_t)count << (S == 256 ? 6 : 7)) + size) << 3;   \
    ((uint8_t *)buf)[size++] = 0x80;                                           \
    if (size > (S == 256 ? 56 : 120)) {                                        \
      std::memset(((uint8_t *)buf) + size, 0, (S == 256 ? 64 : 128) - size);   \
      update(buf);                                                             \
      size = 0;                                                                \
    }                                                                          \
    std::memset(((uint8_t *)buf) + size, 0, (S == 256 ? 56 : 120) - size);     \
    if (S == 256) {                                                            \
      buf[14] = htobe32(length >> 32);                                         \
      buf[15] = htobe32(length & 0xffffffff);                                  \
    } else {                                                                   \
      buf[15] = htobe64(length);                                               \
    }                                                                          \
    update(buf);                                                               \
    for (int i = 0; i < 8; i++)                                                \
      hash[i] = htobe##B(hash[i]);                                             \
  }                                                                            \
  void SHA##S::sha(const uint8_t *buf, int size) {                             \
    int count = 0;                                                             \
    uint##B##_t tmp[16];                                                       \
    while (size >= (S == 256 ? 64 : 128)) {                                    \
      count++;                                                                 \
      std::memcpy(tmp, buf, S == 256 ? 64 : 128);                              \
      update(tmp);                                                             \
      buf += (S == 256 ? 64 : 128);                                            \
      size -= (S == 256 ? 64 : 128);                                           \
    }                                                                          \
    std::memcpy(tmp, buf, size);                                               \
    finish(tmp, size, count);                                                  \
  }                                                                            \
  SHAHMAC##S::SHAHMAC##S(const uint8_t *key, int size) {                       \
    if (size > (S == 256 ? 64 : 128)) {                                        \
      SHA##S sha;                                                              \
      sha.sha(key, size);                                                      \
      std::memcpy(okey_, sha.hash, (S == 256 ? 32 : 64));                      \
      std::memset(okey_ + 8, 0, S == 256 ? 32 : 64);                           \
    } else if (size < (S == 256 ? 64 : 128)) {                                 \
      std::memcpy(okey_, key, size);                                           \
      std::memset(((uint8_t *)okey_) + size, 0, (S == 256 ? 64 : 128) - size); \
    } else {                                                                   \
      std::memcpy(okey_, key, size);                                           \
    }                                                                          \
    for (int i = 0; i < 16; i++) {                                             \
      ikey_[i] = okey_[i] ^ (S == 256 ? 0x36363636 : 0x3636363636363636);      \
      okey_[i] ^= (S == 256 ? 0x5c5c5c5c : 0x5c5c5c5c5c5c5c5c);                \
    }                                                                          \
  }                                                                            \
  void SHAHMAC##S::hmac(const uint8_t *buf, int size) {                        \
    uint##B##_t tmp[16];                                                       \
    std::memcpy(tmp, okey_, S == 256 ? 64 : 128);                              \
    SHA##S osha, isha;                                                         \
    osha.update(tmp);                                                          \
    std::memcpy(tmp, ikey_, S == 256 ? 64 : 128);                              \
    isha.update(tmp);                                                          \
    int count = 1;                                                             \
    while (size > (S == 256 ? 64 : 128)) {                                     \
      count++;                                                                 \
      std::memcpy(tmp, buf, S == 256 ? 64 : 128);                              \
      isha.update(tmp);                                                        \
      buf += (S == 256 ? 64 : 128);                                            \
      size -= (S == 256 ? 64 : 128);                                           \
    }                                                                          \
    std::memcpy(tmp, buf, size);                                               \
    isha.finish(tmp, size, count);                                             \
    osha.finish(isha.hash, S == 256 ? 32 : 64, 1);                             \
    std::memcpy(tag, osha.hash, S == 256 ? 32 : 64);                           \
  }

SHA_IMP(32, 256, 7, 18, 3, 17, 19, 10, 2, 13, 22, 6, 11, 25);
SHA_IMP(64, 512, 1, 8, 7, 19, 61, 6, 28, 34, 39, 14, 18, 41);

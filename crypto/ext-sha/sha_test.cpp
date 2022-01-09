#include <assert.h>
#include <endian.h>
#include <stdint.h>

#include <cstring>

#include "sha.h"

int main() {
  const char *msg = "hello world";
  const char *key = "this is a secret";
  uint32_t etag256[8] = {
      0xdde3100a, 0x1d6e24dd, 0xad38cfd6, 0x05a7cc0f,
      0xa21ba072, 0xd0432c3e, 0x33eb01cb, 0x8ee15a5c,
  };
  for (int i = 0; i < 8; i++)
    etag256[i] = htobe32(etag256[i]);
  uint64_t etag512[8] = {
      0xb2095653a0f66b02, 0xdffcf9c815d79d71, 0xc4969062a7fde276,
      0x08953f22d6938bcf, 0xd76fc99a022f1fb0, 0xc2875a496b531e13,
      0x1c7d7534ce037724, 0x0c20e75b535fd634,
  };
  for (int i = 0; i < 8; i++)
    etag512[i] = htobe64(etag512[i]);

  SHAHMAC256 hmac256((uint8_t *)key, std::strlen(key));
  hmac256.hmac((uint8_t *)msg, std::strlen(msg));
  SHAHMAC512 hmac512((uint8_t *)key, std::strlen(key));
  hmac512.hmac((uint8_t *)msg, std::strlen(msg));

  assert(!std::memcmp(hmac256.tag, etag256, sizeof(etag256)));
  assert(!std::memcmp(hmac512.tag, etag512, sizeof(etag512)));
}

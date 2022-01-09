#ifndef DHS_H
#define DNS_H

#include <stdint.h>
#include <string>
#include <vector>

#define DNS_FLAG_QR 0x8000
#define DNS_FLAG_OPCODE 0x7800
#define DNS_FLAG_OPCODE_REVERSE 0x800
#define DNS_FLAG_OPCODE_SERVER 0x1000
#define DNS_FLAG_AA 0x400
#define DNS_FLAG_TC 0x200
#define DNS_FLAG_RD 0x100
#define DNS_FLAG_RA 0x80
#define DNS_FLAG_Z 0x70
#define DNS_FLAG_RCODE 0xf

#define DNS_TYPE_A 1
#define DNS_TYPE_NS 2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_AAAA 28

#define DNS_CLASS_IN 1

struct DNSQuery {
  std::vector<std::string> name;
  uint16_t type;
  uint16_t cls;

  int from_bytes(const uint8_t *bytes, int size, int cur);
  void push_to_bytes(std::vector<uint8_t> &bytes);
  void print();
};

struct DNSRR {
  DNSQuery query;
  uint32_t ttl;
  std::vector<uint8_t> data;
  std::vector<std::string> name;

  int from_bytes(const uint8_t *bytes, int size, int cur);
  void push_to_bytes(std::vector<uint8_t> &bytes);
  void print();
};

struct DNS {
  uint16_t id;
  uint16_t flags;
  std::vector<DNSQuery> querys;
  std::vector<DNSRR> answers;
  std::vector<DNSRR> authorities;
  std::vector<DNSRR> additionals;

  int from_bytes(const uint8_t *bytes, int size, int cur);
  void push_to_bytes(std::vector<uint8_t> &bytes);
  void print();
};

std::vector<std::string> dns_domain_split(std::string domain);

void dig(std::string ns, std::string domain);

void digot(std::string ns, std::string domain);

void bind(std::string addr, std::string ns);

void bindot(std::string addr, std::string ns);

#endif

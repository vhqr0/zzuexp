#include "dns.h"

#include <stdint.h>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int DNSQuery::from_bytes(const uint8_t *bytes, int size, int cur) {
  int c, in_rec = 0;
  while (true) {
    if (in_rec) {
      if (c + 1 >= size)
        throw std::exception();
      int len = bytes[c++];
      if ((len & 0xc0) == 0xc0) {
        c = ((len & 0x3f) << 8) + bytes[c];
        continue;
      }
      if (len == 0)
        break;
      if (c + len > size)
        throw std::exception();
      name.push_back(std::string((char *)(bytes + c), len));
      c += len;
    } else {
      if (cur + 1 >= size)
        throw std::exception();
      int len = bytes[cur++];
      if ((len & 0xc0) == 0xc0) {
        c = ((len & 0x3f) << 8) + bytes[cur++];
        in_rec = 1;
        continue;
      }
      if (len == 0)
        break;
      if (cur + len > size)
        throw std::exception();
      name.push_back(std::string((char *)(bytes + cur), len));
      cur += len;
    }
  }
  if (cur + 4 > size)
    throw std::exception();
  type = bytes[cur++];
  type = (type << 8) + bytes[cur++];
  cls = bytes[cur++];
  cls = (cls << 8) + bytes[cur++];
  return cur;
}

int DNSRR::from_bytes(const uint8_t *bytes, int size, int cur) {
  cur = query.from_bytes(bytes, size, cur);
  if (cur + 6 > size)
    throw std::exception();
  ttl = bytes[cur++];
  ttl = (ttl << 8) + bytes[cur++];
  ttl = (ttl << 8) + bytes[cur++];
  ttl = (ttl << 8) + bytes[cur++];
  int len = bytes[cur++];
  len = (len << 8) + bytes[cur++];
  if (cur + len > size)
    throw std::exception();
  int c = cur;
  for (int i = 0; i < len; i++)
    data.push_back(bytes[cur++]);
  if (query.type == DNS_TYPE_CNAME || query.type == DNS_TYPE_NS) {
    while (true) {
      if (c + 1 >= size)
        throw std::exception();
      int len = bytes[c++];
      if ((len & 0xc0) == 0xc0) {
        c = ((len & 0x3f) << 8) + bytes[c];
        continue;
      }
      if (len == 0)
        break;
      if (c + len > size)
        throw std::exception();
      name.push_back(std::string((char *)(bytes + c), len));
      c += len;
    }
  }
  return cur;
}

int DNS::from_bytes(const uint8_t *bytes, int size, int cur) {
  if (cur + 12 > size)
    throw std::exception();
  id = bytes[cur++];
  id = (id << 8) + bytes[cur++];
  flags = bytes[cur++];
  flags = (flags << 8) + bytes[cur++];
  uint16_t questions = bytes[cur++];
  questions = (questions << 8) + bytes[cur++];
  uint16_t answer_rrs = bytes[cur++];
  answer_rrs = (answer_rrs << 8) + bytes[cur++];
  uint16_t authority_rrs = bytes[cur++];
  authority_rrs = (authority_rrs << 8) + bytes[cur++];
  uint16_t additional_rrs = bytes[cur++];
  additional_rrs = (additional_rrs << 8) + bytes[cur++];
  querys.clear();
  for (int i = 0; i < questions; i++) {
    DNSQuery query;
    cur = query.from_bytes(bytes, size, cur);
    querys.push_back(query);
  }
  answers.clear();
  for (int i = 0; i < answer_rrs; i++) {
    DNSRR rr;
    cur = rr.from_bytes(bytes, size, cur);
    answers.push_back(rr);
  }
  authorities.clear();
  for (int i = 0; i < authority_rrs; i++) {
    DNSRR rr;
    cur = rr.from_bytes(bytes, size, cur);
    authorities.push_back(rr);
  }
  additionals.clear();
  for (int i = 0; i < additional_rrs; i++) {
    DNSRR rr;
    cur = rr.from_bytes(bytes, size, cur);
    additionals.push_back(rr);
  }
  return cur;
}

void DNSQuery::push_to_bytes(std::vector<uint8_t> &bytes) {
  for (auto &str : name) {
    bytes.push_back(str.length());
    for (auto &c : str)
      bytes.push_back(c);
  }
  bytes.push_back(0);
  bytes.push_back((type & 0xff00) >> 8);
  bytes.push_back(type & 0xff);
  bytes.push_back((cls & 0xff00) >> 8);
  bytes.push_back(cls & 0xff);
}

void DNSRR::push_to_bytes(std::vector<uint8_t> &bytes) {
  query.push_to_bytes(bytes);
  bytes.push_back((ttl & 0xff000000) >> 24);
  bytes.push_back((ttl & 0xff0000) >> 16);
  bytes.push_back((ttl & 0xff00) >> 8);
  bytes.push_back(ttl & 0xff);
  if (!name.empty()) {
    int len = 1;
    for (auto &str : name)
      len += str.length() + 1;
    bytes.push_back((len & 0xff00) >> 8);
    bytes.push_back(len & 0xff);
    for (auto &str : name) {
      bytes.push_back(str.length());
      for (auto &c : str)
        bytes.push_back(c);
    }
    bytes.push_back(0);
  } else {
    int len = data.size();
    bytes.push_back((len & 0xff00) >> 8);
    bytes.push_back(len & 0xff);
    for (auto &c : data)
      bytes.push_back(c);
  }
}

void DNS::push_to_bytes(std::vector<uint8_t> &bytes) {
  uint16_t questions = querys.size();
  uint16_t answer_rrs = answers.size();
  uint16_t authority_rrs = authorities.size();
  uint16_t additional_rrs = additionals.size();
  bytes.push_back((id & 0xff00) >> 8);
  bytes.push_back(id & 0xff);
  bytes.push_back((flags & 0xff00) >> 8);
  bytes.push_back(flags & 0xff);
  bytes.push_back((questions & 0xff00) >> 8);
  bytes.push_back(questions & 0xff);
  bytes.push_back((answer_rrs & 0xff00) >> 8);
  bytes.push_back(answer_rrs & 0xff);
  bytes.push_back((authority_rrs & 0xff00) >> 8);
  bytes.push_back(authority_rrs & 0xff);
  bytes.push_back((additional_rrs & 0xff00) >> 8);
  bytes.push_back(additional_rrs & 0xff);
  for (auto &query : querys)
    query.push_to_bytes(bytes);
  for (auto &answer : answers)
    answer.push_to_bytes(bytes);
  for (auto &authority : authorities)
    authority.push_to_bytes(bytes);
  for (auto &additional : additionals)
    additional.push_to_bytes(bytes);
}

void DNSQuery::print() {
  for (auto &n : name)
    std::cout << n << ".";
  std::cout << "\t";
  switch (type) {
  case DNS_TYPE_A:
    std::cout << "A";
    break;
  case DNS_TYPE_NS:
    std::cout << "NS";
    break;
  case DNS_TYPE_CNAME:
    std::cout << "CNAME";
    break;
  case DNS_TYPE_AAAA:
    std::cout << "AAAA";
    break;
  default:
    std::cout << "UNKNOWN";
  }
}

void DNSRR::print() {
  query.print();
  std::cout << "\t";
  switch (query.type) {
  case DNS_TYPE_CNAME:
  case DNS_TYPE_NS:
    for (auto &str : name)
      std::cout << str << ".";
    break;
  default:
    for (auto &i : data)
      std::cout << (int)i << ".";
    break;
  }
}

void DNS::print() {
  std::cout << "id: " << id << std::endl << "flags: " << flags << std::endl;
  std::cout << "querys:" << std::endl;
  for (auto &query : querys) {
    query.print();
    std::cout << "\n";
  }
  std::cout << "answers:" << std::endl;
  for (auto &answer : answers) {
    answer.print();
    std::cout << "\n";
  }
  std::cout << "authorities:" << std::endl;
  for (auto &authority : authorities) {
    authority.print();
    std::cout << "\n";
  }
  std::cout << "additionals:" << std::endl;
  for (auto &additional : additionals) {
    additional.print();
    std::cout << "\n";
  }
}

std::vector<std::string> dns_domain_split(std::string domain) {
  std::vector<std::string> name;
  std::string s;
  for (auto &c : domain) {
    if (c == '.') {
      name.push_back(s);
      s.clear();
    } else {
      s += c;
    }
  }
  name.push_back(s);
  return name;
}

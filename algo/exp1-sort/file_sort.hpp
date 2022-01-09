#ifndef FILE_SORT_HPP
#define FILE_SORT_HPP

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "Sorter.hpp"

template <typename T> void gen_vector(std::vector<T> &v, int size) {
  std::srand(std::time(NULL));
  for (int i = 0; i < size; i++) {
    T a;
    for (int j = 0; j < (sizeof(T) / sizeof(int)); j++)
      ((int *)&a)[j] = std::rand();
    v.push_back(a);
  }
}

template <>
void gen_vector<std::string>(std::vector<std::string> &v, int size) {
  std::srand(std::time(NULL));
  for (int i = 0; i < size; i++) {
    std::string a;
    for (int j = rand() % 16; j >= 0; j--)
      a += char('a' + rand() % 26);
    v.push_back(a);
  }
}

template <typename T> void write_vector(std::string file, std::vector<T> &v) {
  std::ofstream ofs(file);
  for (auto &i : v)
    ofs << i << std::endl;
}

template <typename T> void read_vector(std::string file, std::vector<T> &v) {
  std::ifstream ifs(file);
  T a;
  while (ifs >> a)
    v.push_back(a);
}

template <typename T> void quick_sort_vector(std::vector<T> &v) {
  Sorter<T>().quick_sort(&v[0], v.size());
}

template <typename T> void heap_sort_vector(std::vector<T> &v) {
  Sorter<T>().heap_sort(&v[0], v.size());
}

template <typename T>
void handle_vector(
    int argc, char **argv,
    std::function<void(std::vector<T> &, int)> gen = gen_vector<T>) {
  if (argc <= 4) {
    std::cerr << "wrong argument" << std::endl;
    std::exit(-1);
  }
  std::string cmd(argv[2]);
  if (cmd == "gen") {
    std::vector<T> v;
    gen(v, 1 << atoi(argv[3]));
    write_vector<T>(std::string(argv[4]), v);
  } else if (cmd == "quick") {
    std::vector<T> v;
    read_vector<T>(argv[3], v);
    auto beg = std::clock();
    quick_sort_vector<T>(v);
    auto end = std::clock();
    write_vector<T>(argv[4], v);
    std::cout << end - beg << "us" << std::endl;
  } else if (cmd == "heap") {
    std::vector<T> v;
    read_vector<T>(argv[3], v);
    auto beg = std::clock();
    heap_sort_vector<T>(v);
    auto end = std::clock();
    write_vector<T>(argv[4], v);
    std::cout << end - beg << "us" << std::endl;
  } else {
    std::cerr << "wrong argument" << std::endl;
    std::exit(-1);
  }
}

#endif

#include <iostream>

#include "file_sort.hpp"
#include "mysql_sort.h"
#include "usage_reporter.h"

using namespace std;

int main(int argc, char **argv) {
  set_usage_reporter();
  if (argc <= 2) {
    cerr << "wrong argument" << endl;
    return -1;
  }
  string type(argv[1]);
  if (type == "ints") {
    handle_vector<int>(argc, argv);
  } else if (type == "flts") {
    handle_vector<double>(argc, argv);
  } else if (type == "strs") {
    handle_vector<string>(argc, argv);
  } else if (type == "dbstrs") {
    handle_dbstrs(argc, argv);
  } else {
    cerr << "wrong argument" << endl;
    return -1;
  }
  return 0;
}

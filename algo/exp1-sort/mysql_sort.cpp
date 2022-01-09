#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include <mysql/mysql.h>

#include "Sorter.hpp"

#define STRING_LEN 16

using namespace std;

extern char **environ;

void db_connect(MYSQL *mysql) {
  mysql_init(mysql);
  if (!mysql_real_connect(
          mysql, getenv("EXP1_MYSQL_HOST"), getenv("EXP1_MYSQL_USER"),
          getenv("EXP1_MYSQL_PASSWORD"), getenv("EXP1_MYSQL_DATABASE"),
          atoi(getenv("EXP1_MYSQL_PORT")), NULL, CLIENT_FOUND_ROWS)) {
    cerr << "can't connect to database: " << mysql_error(mysql) << endl;
    exit(-1);
  }
}

void db_create(MYSQL *mysql, string name = "strs", bool rename = true) {
  if (rename) {
    string old = name + "_old";
    mysql_query(mysql, ("drop table " + old).c_str());
    mysql_query(mysql, ("rename table " + name + " to " + old).c_str());
  }
  if (mysql_query(mysql, ("create table " + name +
                          "(id int auto_increment primary key,"
                          "value varchar(" +
                          to_string(STRING_LEN) +
                          ") not null,"
                          "rank_ int)engine=innodb default charset=utf8")
                             .c_str())) {
    cerr << "can't create table: " << mysql_error(mysql) << endl;
    exit(-1);
  }
}

void db_gen_strs(MYSQL *mysql, int size, string name = "strs") {
  srand(time(NULL));
  vector<string> strs;
  for (int i = 0; i < size; i++) {
    string a;
    for (int j = 0; j < STRING_LEN; j++)
      a += char('a' + rand() % 26);
    strs.push_back(a);
  }
  MYSQL_STMT *stmt = mysql_stmt_init(mysql);
  string statement = "insert into " + name + " (value) values (?)";
  if (mysql_stmt_prepare(stmt, statement.c_str(), statement.length())) {
    cerr << "compile statement failed: " << mysql_stmt_error(stmt) << endl;
    exit(-1);
  }
  MYSQL_BIND bind;
  bind.buffer_type = MYSQL_TYPE_STRING;
  bind.buffer_length = STRING_LEN;
  bind.is_null = 0;
  for (auto &str : strs) {
    unsigned long str_len = str.length();
    bind.buffer = &str[0];
    bind.length = &str_len;
    if (mysql_stmt_bind_param(stmt, &bind)) {
      cerr << "bind statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
    if (mysql_stmt_execute(stmt)) {
      cerr << "execute statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
  }
  mysql_stmt_close(stmt);
  mysql_commit(mysql);
}

void db_write_strs_with_id(MYSQL *mysql, vector<pair<int, string>> &strs,
                           string name = "strs") {
  MYSQL_STMT *stmt = mysql_stmt_init(mysql);
  string statement =
      "insert into " + name + " (id, value, rank_) values (?, ?, ?)";
  if (mysql_stmt_prepare(stmt, statement.c_str(), statement.length())) {
    cerr << "compile statement failed: " << mysql_stmt_error(stmt) << endl;
    exit(-1);
  }
  MYSQL_BIND bind[3];
  memset(bind, 0, sizeof(bind));
  bind[0].buffer_type = MYSQL_TYPE_LONG;
  bind[0].is_null = 0;
  bind[0].length = 0;
  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer_length = STRING_LEN;
  bind[1].is_null = 0;
  bind[2].buffer_type = MYSQL_TYPE_LONG;
  bind[2].is_null = 0;
  bind[2].length = 0;
  string prev_value = "";
  int prev_rank = 1;
  for (int i = 0; i < strs.size(); i++) {
    bind[0].buffer = (char *)&strs[i].first;
    unsigned long str_len = strs[i].second.length();
    bind[1].buffer = &strs[i].second[0];
    bind[1].length = &str_len;
    int rank = strs[i].second == prev_value ? prev_rank : i + 1;
    bind[2].buffer = (char *)&rank;
    if (mysql_stmt_bind_param(stmt, bind)) {
      cerr << "bind statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
    if (mysql_stmt_execute(stmt)) {
      cerr << "execute statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
    prev_value = strs[i].second;
    prev_rank = rank;
  }
  mysql_stmt_close(stmt);
  mysql_commit(mysql);
}

void db_read_strs_with_id(MYSQL *mysql, vector<pair<int, string>> &strs,
                          string name = "strs") {
  if (mysql_query(mysql, ("select id, value from " + name).c_str())) {
    cerr << "can't select from database: " << mysql_error(mysql);
    exit(-1);
  }
  MYSQL_RES *res = mysql_store_result(mysql);
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
    strs.push_back({atoi(row[0]), row[1]});
  mysql_free_result(res);
}

void db_quick_sort_strs_with_id(vector<pair<int, string>> &strs) {
  Sorter<pair<int, string>>([](pair<int, string> a,
                               pair<int, string> b) -> bool {
    return a.second <= b.second;
  }).quick_sort(&strs[0], strs.size());
}

void db_heap_sort_strs_with_id(vector<pair<int, string>> &strs) {
  Sorter<pair<int, string>>([](pair<int, string> a,
                               pair<int, string> b) -> bool {
    return a.second <= b.second;
  }).heap_sort(&strs[0], strs.size());
}

class DBN {
private:
  MYSQL *mysql;
  string name, drop_query, select1_query, delete1_query;

public:
  DBN(MYSQL *mysql, int n, int page_size, string prefix = "strs")
      : mysql(mysql), name(prefix + to_string(n)),
        drop_query("drop table " + name),
        select1_query("select value, rank_ from " + name + " limit 1"),
        delete1_query("delete from " + name + " limit 1") {
    db_create(mysql, name, false);
    vector<pair<int, string>> strs;
    db_read_strs_with_id(mysql, strs,
                         prefix + " limit " +
                             to_string(page_size * (n - 1)) + ", " +
                             to_string(page_size));
    db_heap_sort_strs_with_id(strs);
    string write_statement =
        "insert into " + name + " (value, rank_) values (?, ?)";
    MYSQL_STMT *write_stmt = mysql_stmt_init(mysql);
    if (mysql_stmt_prepare(write_stmt, write_statement.c_str(),
                           write_statement.length())) {
      cerr << "compile statement failed: " << mysql_stmt_error(write_stmt)
           << endl;
      exit(-1);
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer_length = STRING_LEN;
    bind[0].is_null = 0;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].is_null = 0;
    bind[1].length = 0;
    for (auto &str : strs) {
      unsigned long str_len = str.second.length();
      bind[0].buffer = &str.second[0];
      bind[0].length = &str_len;
      bind[1].buffer = (char *)&str.first;
      if (mysql_stmt_bind_param(write_stmt, bind)) {
        cerr << "bind statement failed: " << mysql_stmt_error(write_stmt)
             << endl;
        exit(-1);
      }
      if (mysql_stmt_execute(write_stmt)) {
        cerr << "execute statement failed: " << mysql_stmt_error(write_stmt)
             << endl;
        exit(-1);
      }
    }
    mysql_stmt_close(write_stmt);
    mysql_commit(mysql);
  }

  pair<int, string> *pop() {
    if (mysql_query(mysql, select1_query.c_str())) {
      cerr << "can't select from database: " << mysql_error(mysql);
      exit(-1);
    }
    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    pair<int, string> *p =
        row ? new pair<int, string>{atoi(row[1]), row[0]} : nullptr;
    mysql_free_result(res);
    if (p && mysql_query(mysql, delete1_query.c_str())) {
      cerr << "can't delete from database: " << mysql_error(mysql);
      exit(-1);
    }
    return p;
  }

  ~DBN() {
    if (mysql_query(mysql, drop_query.c_str())) {
      cerr << "can't drop table: " << mysql_error(mysql);
      exit(-1);
    }
  }
};

void external_sort(MYSQL *mysql, int page_size, string name = "strs") {
  list<pair<DBN *, pair<int, string> *>> dbs;
  if (mysql_query(mysql, ("select count(*) from " + name).c_str())) {
    cerr << "can't select from database: " << mysql_error(mysql);
    exit(-1);
  }
  MYSQL_RES *res = mysql_store_result(mysql);
  MYSQL_ROW row = mysql_fetch_row(res);
  if (!row) {
    cerr << "select count from database return NULL" << endl;
    exit(-1);
  }
  int count = atoi(row[0]);
  mysql_free_result(res);
  int cur = 1;
  while (count > 0) {
    dbs.push_front({new DBN(mysql, cur++, page_size, name), nullptr});
    count -= page_size;
  }
  for (auto i = dbs.begin(); i != dbs.end(); i++) {
    i->second = i->first->pop();
    if (!i->second) {
      delete i->first;
      dbs.erase(i);
    }
  }
  db_create(mysql, name);
  MYSQL_STMT *stmt = mysql_stmt_init(mysql);
  string statement = "insert into " + name + " (id, value, rank_) values (?, ?, ?)";
  if (mysql_stmt_prepare(stmt, statement.c_str(), statement.length())) {
    cerr << "compile statement failed: " << mysql_stmt_error(stmt) << endl;
    exit(-1);
  }
  MYSQL_BIND bind[3];
  memset(bind, 0, sizeof(bind));
  bind[0].buffer_type = MYSQL_TYPE_LONG;
  bind[0].is_null = 0;
  bind[0].length = 0;
  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer_length = STRING_LEN;
  bind[1].is_null = 0;
  bind[2].buffer_type = MYSQL_TYPE_LONG;
  bind[2].is_null = 0;
  bind[2].length = 0;
  string prev_value = "";
  int prev_rank = 1;
  int true_rank = 1;
  while (!dbs.empty()) {
    auto minp = dbs.begin();
    for (auto i = dbs.begin(); i != dbs.end(); i++)
      if (i->second->second < minp->second->second)
        minp = i;
    bind[0].buffer = (char *)&minp->second->first;
    unsigned long str_len = minp->second->second.length();
    bind[1].buffer = &minp->second->second[0];
    bind[1].length = &str_len;
    int rank = minp->second->second == prev_value ? prev_rank : true_rank;
    bind[2].buffer = (char *)&rank;
    if (mysql_stmt_bind_param(stmt, bind)) {
      cerr << "bind statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
    if (mysql_stmt_execute(stmt)) {
      cerr << "execute statement failed: " << mysql_stmt_error(stmt) << endl;
      exit(-1);
    }
    mysql_commit(mysql);
    prev_value = minp->second->second;
    prev_rank = rank;
    true_rank++;
    delete minp->second;
    minp->second = minp->first->pop();
    if (!minp->second) {
      delete minp->first;
      dbs.erase(minp);
    }
  }
  mysql_stmt_close(stmt);
}

void handle_dbstrs(int argc, char **argv) {
  MYSQL mysql;
  db_connect(&mysql);
  string cmd(argv[2]);
  if (cmd == "gen") {
    if (argc <= 3) {
      cerr << "wrong argument" << endl;
      exit(-1);
    }
    db_gen_strs(&mysql, 1 << atoi(argv[3]));
  } else if (cmd == "quick") {
    vector<pair<int, string>> strs;
    db_read_strs_with_id(&mysql, strs);
    auto beg = clock();
    db_quick_sort_strs_with_id(strs);
    auto end = clock();
    db_create(&mysql);
    db_write_strs_with_id(&mysql, strs);
    cout << end - beg << "us" << endl;
  } else if (cmd == "heap") {
    vector<pair<int, string>> strs;
    db_read_strs_with_id(&mysql, strs);
    auto beg = clock();
    db_heap_sort_strs_with_id(strs);
    auto end = clock();
    db_create(&mysql);
    db_write_strs_with_id(&mysql, strs);
    cout << end - beg << "us" << endl;
  } else if (cmd == "external") {
    if (argc <= 3) {
      cerr << "wrong argument" << endl;
      exit(-1);
    }
    auto beg = clock();
    external_sort(&mysql, 1 << atoi(argv[3]));
    auto end = clock();
    cout << end - beg << "us" << endl;
  } else {
    cerr << "wrong argument" << endl;
    exit(-1);
  }
  mysql_close(&mysql);
}

#include "smt.h"

#include <map> 
#include <fstream> 
#include <iostream> 

#include "benchmarking.h"

using namespace std;

bool enable_smt_logging = false;
extern int run_id;

thread_local map<string, pair<long long, long long>> stats;

namespace smt {

void _solver::log_smtlib(
      long long ms,
      std::string const& res)
{
  static int log_num = 1;

  string filename = "./logs/smtlib/log." + to_string(run_id) + "." + to_string(log_num) + ".z3";
  log_num++;

  ofstream myfile;
  myfile.open(filename);
  myfile << "; time: " << ms << " ms" << endl;

  myfile << "; result: " << res;

  myfile << "; " << log_info << endl;
  myfile << endl;

  dump(myfile);

  myfile.close();

  cout << "logged smtlib to " << filename << endl;
}

void log_to_stdout(long long ms, bool is_cvc4,
    string const& log_info, string const& res) {
  string l;
  if (log_info == "") {
    l = "[???]";
  } else {
    l = "[" + log_info + "]";
  }

  string key;
  if (is_cvc4) {
    cout << "SMT result (cvc4) ";
    key = "cvc4 ";
  } else {
    cout << "SMT result (z3) ";
    key = "z3 ";
  }
  cout << l << " : " << res << " " << ms << " ms" << endl;

  for (int i = 0; i < 2; i++) {
    string newkey;
    if (i == 0) newkey = key + l + " " + res;
    else newkey = key + "TOTAL" + " " + res;

    while (newkey.size() < 58) newkey += " ";
    if (stats.find(newkey) == stats.end()) {
      stats.insert(make_pair(newkey, make_pair(0, 0)));
    }
    stats[newkey].first++;
    stats[newkey].second += ms;
  }
}

void dump_smt_stats() {
  for (auto& p : stats) {
    long long num = p.second.first;
    long long ms = p.second.second;
    long long avg = ms / num;
    cout << p.first << " total " << ms << " ms over " << num
         << " ops, average is " << avg << " ms" << endl;
  }
}

}

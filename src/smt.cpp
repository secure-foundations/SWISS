#include "smt.h"

#include <fstream> 
#include <map> 

#include "benchmarking.h"

using namespace std;

bool enable_smt_logging = false;
extern int run_id;

namespace smt {

thread_local map<string, pair<long long, long long>> stats;

namespace smt {

void solver::log_smtlib(
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

#ifdef SMT_CVC4

string res_to_string(CVC4::Result::Sat res) {
  if (res == CVC4::Result::SAT) {
    return "sat";
  } else if (res == CVC4::Result::UNSAT) {
    return "unsat";
  } else if (res == CVC4::Result::SAT_UNKNOWN) {
    return "timeout/unknown";
  } else {
    assert(false);
  }
}

bool solver::check_sat()
{
  //cout << "check_sat" << endl;
  auto t1 = now();
  CVC4::Result::Sat res = smt.checkSat().isSat();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  assert (res == CVC4::Result::SAT || res == CVC4::Result::UNSAT);
  //cout << "res is " << (res == CVC4::Result::SAT ? "true" : "false") << endl;
  return res == CVC4::Result::SAT;
}

#endif

/*void solver::log_smtlib(
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

  #ifdef SMT_CVC4
  smt.setInfo("smt-lib-version", 2);
  myfile << "not implemented" << endl;
  #else
  myfile << z3_solver << endl;
  #endif

  myfile.close();

  cout << "logged smtlib to " << filename << endl;
}*/

}

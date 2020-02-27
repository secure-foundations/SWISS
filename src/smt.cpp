#include "smt.h"

#include <fstream> 

#include "benchmarking.h"

using namespace std;

bool enable_smt_logging = false;
extern int run_id;

namespace smt {

bool solver::check_sat()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  if (enable_smt_logging) {
    long long ms = as_ms(t2 - t1);
    log_smtlib(ms, res);
  }

  assert (res == z3::sat || res == z3::unsat);
  return res == z3::sat;
}

bool solver::is_sat_or_unknown()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  if (enable_smt_logging) {
    long long ms = as_ms(t2 - t1);
    log_smtlib(ms, res);
  }

  assert (res == z3::sat || res == z3::unsat || res == z3::unknown);
  return res == z3::sat || res == z3::unknown;
}

bool solver::is_unsat_or_unknown()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  if (enable_smt_logging) {
    long long ms = as_ms(t2 - t1);
    log_smtlib(ms, res);
  }

  assert (res == z3::sat || res == z3::unsat || res == z3::unknown);
  return res == z3::unsat || res == z3::unknown;
}

void solver::log_smtlib(
    long long ms,
    z3::check_result res)
{
  static int log_num = 1;

  string filename = "./logs/smtlib/log." + to_string(run_id) + "." + to_string(log_num) + ".z3";
  log_num++;

  ofstream myfile;
  myfile.open(filename);
  myfile << "; time: " << ms << " ms" << endl;

  if (res == z3::sat) {
    myfile << "; result: sat";
  } else if (res == z3::unsat) {
    myfile << "; result: unsat";
  } else if (res == z3::unknown) {
    myfile << "; result: timeout/unknown";
  }

  myfile << "; " << log_info << endl;
  myfile << endl;
  myfile << z3_solver << endl;
  myfile.close();

  cout << "logged smtlib to " << filename << endl;
}

}
